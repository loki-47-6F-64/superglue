#import "ThreadImpl.h"
#import "uThreadCallback.h"
@implementation ThreadImpl

- (void) run:(uThreadCallback*)callBack {
  [callBack run];
}

- (void) create:(uThreadCallback*)callBack {
  [self performSelectorInBackground:@selector(run:) withObject:(callBack)];
}

@end