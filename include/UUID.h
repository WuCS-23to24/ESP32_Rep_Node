//
//    FILE: UUID.h
//  AUTHOR: Rob Tillaart
// VERSION: 0.1.6
//    DATE: 2022-06-14
// PURPOSE: Arduino Library for generating UUID's
//     URL: https://github.com/RobTillaart/UUID
//          https://en.wikipedia.org/wiki/Universally_unique_identifier
//
// e.g.     20d24650-d900-e34f-de49-8964ab3eb46d

#ifndef BD282200_53D1_42C6_9E9C_17055ECFEFD6
#define BD282200_53D1_42C6_9E9C_17055ECFEFD6

#include "Arduino.h"
#include "Printable.h"

#define UUID_LIB_VERSION (F("0.1.6"))

enum UUID_MODE
{
    VARIANT4,
    RANDOM
};

/////////////////////////////////////////////////
//
//  CLASS VERSION
//
class UUID : public Printable
{
  public:
    UUID();

    //  at least one seed value is mandatory, two is better.
    void seed(uint32_t s1, uint32_t s2 = 0);
    //  generate a new UUID
    void generate();
    //  make a UUID string
    char *toCharArray();

    //  MODE
    void setVariant4Mode();
    void setRandomMode();
    uint8_t getMode();

    //  Printable interface
    size_t printTo(Print &p) const;

  private:
    //  Marsaglia 'constants' + function
    uint32_t _m_w = 1;
    uint32_t _m_z = 2;
    uint32_t _random();

    //  UUID in string format
    char _buffer[37];
    uint8_t _mode = UUID_MODE::VARIANT4;
};

//  -- END OF FILE --

#endif /* BD282200_53D1_42C6_9E9C_17055ECFEFD6 */
