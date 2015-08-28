#import "Common.h"
#import "uCommonInterface.h"
#import "LogImpl.h"
#import "ThreadImpl.h"
#import "FileImpl.h"

@implementation SuperGlueCommon
static BOOL loaded = false;

+ (void) init {
  if(!loaded) {
    [uCommonInterface config:[[LogImpl alloc] init] threadManager:[[ThreadImpl alloc] init] fileManager:[[FileImpl alloc] init]];
    loaded = true;
  }
}

@end
