# VoodooI2CHID

This is a fork with a fix for the Precision Trackpad and Touch Screen Trackpad satellites to work correctly for devices that can work with multiple modes (trackpad or touch screen).

An example of such a device would be the Screenpads in many ASUS laptops. 

Without this fix, such devices only work in single finger mode.

The root cause is in the parsing of the report descriptors to maintain a list of supproted elements to use when an input report is received. The original code assumed a single mode for any device. But this could result in incorrect caching of element objects for a record descriptor which contained common elements in multiple modes.

The fix allows the Precision Trackpad and Touch Screen Event Drivers to narrow the parsing of record descriptors to their respective modes only.

This has solved the multi-touch problem with Screenpad devices such as The Goodix GDX1515 amongst others.
