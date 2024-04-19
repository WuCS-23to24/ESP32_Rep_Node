#include <stdint.h>
#include <Arduino.h>

uint16_t frame_data_decoded[100] = {};
uint16_t getParity16(uint16_t n);
uint8_t adc_pin = A0;//digitalPinToPinName(A0);


void acoustic_receive_loop(void *args)
{
    analogReadResolution(12);
    dacDisable(adc_pin);
    uint16_t frame_index = 0;
    uint16_t frame_parity_error_counter = 0;
    uint16_t frame_mismatch_counter = 0;

    uint16_t decoded_data = 0;
    uint16_t decode_bit_position = 0;

    const int symbol_period = 100;

    unsigned long time_start = 0;
    unsigned long time_end = 0;
    unsigned long time_delta = 0;

    float half_periods = 0.0;
    int half_periods_remaining = 0;
    
    bool start_seq_flag = 0;
    bool stop_seq_flag = 0;
    bool frame_successfully_decoded = 0;

    const int filter_size = 16;
    const int filter_threshold_upper = filter_size*1600;
    const int filter_threshold_lower = filter_size*1400;
    
    uint16_t filter[filter_size] = {0};
    uint32_t filter_sum = 0;
    uint16_t filter_output = 0;
    uint16_t filter_output_prev = 0;
    
    while(1)
    {
        // STEP 1: SAMPLE AND FILTER
        filter_sum = 0;
        for (int i = 0; i < filter_size-1; i++)
        {
            filter_sum += filter[i];
            filter[i] = filter[i+1];
        }
        filter[filter_size-1] = analogRead(adc_pin);
        
        // STEP 2: EDGE DETECTION
        filter_output_prev = filter_output;
        if (filter_sum < filter_threshold_lower)
        {
            filter_output = 0;
        }
        else if (filter_sum > filter_threshold_upper )
        {
            filter_output = 1;
        }

        // STEP 3A: PROCESS RISING EDGE
        if (filter_output > filter_output_prev)
        {
            time_end = millis();
            time_delta = time_end-time_start;
            time_start = time_end;

            if (time_delta > 1000)
            {
                Serial.printf("\nNew sequence detected.\n");
            }
            else
            {
                half_periods = float(time_delta)/float(symbol_period/2); // consider changing to not require floats
                half_periods_remaining = round(half_periods);
                if (half_periods_remaining%2 == 1)
                {
                    half_periods_remaining--;
                    stop_seq_flag = 1; // the existence of the stop bit is determined here, but flag is used because 0s need to be counted before stopping decode
                }
                while (half_periods_remaining > 0)
                {
                    half_periods_remaining -= 2;
                    Serial.printf("0 ");
                    decode_bit_position++;
                }
                if (stop_seq_flag)
                {
                    Serial.printf("\nStop sequence detected.\n");
                    if (!start_seq_flag || decode_bit_position < 16)
                    {
                        Serial.printf("Stop sequence did not follow start sequence and 16 bits as expected. Frame discarded.\n");
                        decoded_data = 0;
                        decode_bit_position = 0;
                    }
                    else
                    {
                        frame_successfully_decoded = 1;
                    }
                    start_seq_flag = 0;
                }
            }
        }

        // STEP 3B: PROCESS FALLING EDGE
        else if (filter_output < filter_output_prev)
        {
            time_end = millis();
            time_delta = time_end-time_start;
            time_start = time_end;

            if (stop_seq_flag == 1)
            {
                stop_seq_flag = 0;
            }
            else
            {
                half_periods = float(time_delta)/float(symbol_period/2); // consider changing to not require floats
                half_periods_remaining = round(half_periods);
                if (half_periods_remaining%2 == 1)
                {
                    Serial.printf("Start sequence detected.\n");
                    start_seq_flag = 1;
                    half_periods_remaining--;
                    decoded_data = 0;
                    decode_bit_position = 0;
                }
                while (half_periods_remaining > 0)
                {
                    half_periods_remaining -= 2;
                    Serial.printf("1 ");
                    bitSet(decoded_data, decode_bit_position);
                    decode_bit_position++;
                }
            }  
        }

        // STEP 4: PROCESS DECODED FRAME
        if (frame_successfully_decoded && frame_index < 100)
        {
            frame_data_decoded[frame_index] = decoded_data;
            Serial.printf("Decoded frame %i. Frame data: 0x%08X. ");
            if (getParity16(frame_data_decoded[frame_index]))
            {
                Serial.printf("Parity incorrect. Error detected.\n");
                frame_parity_error_counter++;
            }
            else
            {
                Serial.printf("Parity correct.\n");
            }
            frame_index++;
        }

        // STEP 5: PROCESS DECODED MESSAGE 
        if (frame_index > 99)
        {
            Serial.printf("Sequence of 100 frames receieved. Writing to file and comparing to expected frame sequence.\n");
            // to do: write results to a file

            /*for (int i = 0; i < 100; i++)
            {
                if (frame_data_decoded[i] != frame_data[i])
                {
                    frame_mismatch_counter++;
                }
            } */
            Serial.printf("Frame parity errors detected: %i. Frame mismatch errors detected: %i.\n", frame_parity_error_counter, frame_mismatch_counter);
            frame_parity_error_counter = 0;
            frame_mismatch_counter = 0; 
            frame_index = 0;
        }
    }
}

uint16_t getParity16(uint16_t n) // returns 0x00 if even parity, 0x01 if odd parity
{
    uint16_t m = n ^ (n >> 1);
    m = m ^ (m >> 2);
    m = m ^ (m >> 4);
    m = m ^ (m >> 8);
    m = m & 0x0001;
    return m;
}
