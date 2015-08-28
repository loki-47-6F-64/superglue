#import "LogImpl.h"

NSString *toString(uLogSeverity severity) {
  switch (severity) {
    case uLogSeverityDebug:
      return @"Debug";
    case uLogSeverityInfo:
      return @"Info";
    case uLogSeverityWarn:
      return @"Warn";
    case uLogSeverityError:
      return @"Error";
  }
  
  return @"Unknown";
}

@implementation LogImpl

- (void)log:(uLogSeverity)severity message:(NSString *)message {
  NSLog(@"CPP_LOG/%@: %@", toString(severity), message);
}

@end