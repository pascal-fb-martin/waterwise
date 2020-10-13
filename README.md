# waterwise

## Overview

A micro service to interface with the bewaterwise.com watering index RSS feed

This project depends on [echttp](https://github.com/pascal-fb-martin/echttp) and [houseportal](https://github.com/pascal-fb-martin/houseportal). It accepts all standard options of echttp and the houseportal client runtime. See these two projects for more information.

## Interface

The waterwise service accepts the /waterwise/status HTTP URL and returns the watering index information in the JSON format, for example:
```
{
  "waterwise": {
    "timestamp": 1602561976,
    "host": "andresy",
    "status": {
      "origin": "http://www.bewaterwise.com/RSS/rsswi.xml",
      "state": "a",
      "index": 91,
      "received": 1602561782,
      "calculated": 1603263600
    }
  }
}
```
The timestamp field represents the time of the request, the host field represents the name of the server hosting this micro-service.

The origin field represent the URL used to obtain the index. The state field is either 'u' (not yet initialized), 'f' (failed to access the bewaterwise.com web site), 'e' (malformed JSON data received) or 'a' (active). Except when active, an error field provides a description of the error.

The index field is a percentage (which can be above 100), the received field indicate when this index was obtained, and the calculated field indicates when the bewaterwise.com site calculated the index. (This service use the weekly index.)
