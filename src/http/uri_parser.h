#ifndef _URI_PARSER_H_
#define _URI_PARSER_H_

#include <Arduino.h>

namespace Uri
{
  enum Scheme {
    http = 0,
    https = 1
  };

  struct Uri {
    Scheme scheme;
    String domain;
    String path;
    int port;
  };

  class UriParser {
    public:
      static bool parse(const String& _uri, Uri& u);

    private:
      static const String kSchemeHttp PROGMEM;
      static const String kSchemeHttps PROGMEM;
      static const String kSchemeDivider PROGMEM;
      static const String kPathDivider PROGMEM;
      static const String kPortDivider PROGMEM;
  };
}
#endif // _URI_PARSER_H_
