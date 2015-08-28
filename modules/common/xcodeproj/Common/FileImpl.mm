#import "FileImpl.h"

@implementation FileImpl

- (NSString*)resourceTxt:(NSString *)fileName {
  NSString *trueName = [fileName stringByDeletingPathExtension];
  NSString *ext = [fileName pathExtension];
  
  NSString *path = [[NSBundle mainBundle] pathForResource:trueName ofType:ext];
  
  NSError __autoreleasing *err;
  return [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:&err];
}

- (NSData*)resourceBin:(NSString *)fileName {
  NSString *trueName = [fileName stringByDeletingPathExtension];
  NSString *ext = [fileName pathExtension];
  
  NSString *path = [[NSBundle mainBundle] pathForResource:trueName ofType:ext];
  
  return [NSData dataWithContentsOfFile:path];
}

- (NSString*) homeDir {
  return [NSString stringWithFormat:@"%@/", [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]];
}

@end