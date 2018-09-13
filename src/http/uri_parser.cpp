#include "uri_parser.h"

namespace Uri {
  const String UriParser::kSchemeHttp PROGMEM = "http";
  const String UriParser::kSchemeHttps PROGMEM = "https";
  const String UriParser::kSchemeDivider PROGMEM = "://";
  const String UriParser::kPathDivider PROGMEM = "/";
  const String UriParser::kPortDivider PROGMEM = ":";

  bool UriParser::parse(const String& _uri, Uri& u) {
    String uri = _uri;

    uri.toLowerCase();

    // Find where authority part of URI starts. Everything up to this is
    // part of the scheme.
    const int authorityOffset = uri.indexOf(kSchemeDivider);
    if(authorityOffset == kSchemeHttp.length() && uri.substring(0, authorityOffset) == kSchemeHttp) {
      u.scheme = http;
      u.port = 80;
    } else if(authorityOffset == kSchemeHttps.length() && uri.substring(0, authorityOffset) == kSchemeHttps) {
      u.scheme = https;
      u.port = 443;
    } else {
      return false;
    }

    // Get the domain part out of the URI (called authority),
    // everything after :// and before /. We keep it simple, no fancy
    // check on valid domain naimes.
    const int pathOffset = uri.indexOf(kPathDivider, authorityOffset + kSchemeDivider.length());
    const String authority = uri.substring(authorityOffset + kSchemeDivider.length(), pathOffset);

    // Remaining of the string, including query.
    u.path = uri.substring(pathOffset, uri.length());

    // If there's a custom port specified, retreive that and set it.
    // Otherwise the authrority == domain.
    const int portOffset = authority.indexOf(kPortDivider);
    if(portOffset > 0) {
      const String portString = authority.substring(portOffset + kPortDivider.length(), authority.length());
      u.port = portString.toInt();
      u.domain = authority.substring(0, portOffset);
    } else {
      u.domain = authority;
    }

    return true;
  }
}
