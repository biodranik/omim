#import "MapsAppDelegate.h"
#import "MWMFrameworkListener.h"

#include "Framework.h"

#include "std/mutex.hpp"

namespace
{
using TObserver = id<MWMFrameworkObserver>;
using TRouteBuildingObserver = id<MWMFrameworkRouteBuilderObserver>;
using TMyPositionObserver = id<MWMFrameworkMyPositionObserver>;
using TStorageObserver = id<MWMFrameworkStorageObserver>;
using TDrapeObserver = id<MWMFrameworkDrapeObserver>;

using TObservers = NSHashTable<__kindof TObserver>;

Protocol * pRouteBuildingObserver = @protocol(MWMFrameworkRouteBuilderObserver);
Protocol * pMyPositionObserver = @protocol(MWMFrameworkMyPositionObserver);
Protocol * pStorageObserver = @protocol(MWMFrameworkStorageObserver);
Protocol * pDrapeObserver = @protocol(MWMFrameworkDrapeObserver);

using TLoopBlock = void (^)(__kindof TObserver observer);

void loopWrappers(TObservers * observers, TLoopBlock block)
{
  dispatch_async(dispatch_get_main_queue(), ^
  {
    for (TObserver observer in observers)
    {
      if (observer)
        block(observer);
    }
  });
}
} // namespace

@interface MWMFrameworkListener ()

@property (nonatomic) TObservers * routeBuildingObservers;
@property (nonatomic) TObservers * myPositionObservers;
@property (nonatomic) TObservers * storageObservers;
@property (nonatomic) TObservers * drapeObservers;

@property (nonatomic, readwrite) location::EMyPositionMode myPositionMode;

@end

@implementation MWMFrameworkListener

+ (MWMFrameworkListener *)listener
{
  static MWMFrameworkListener * listener;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{ listener = [[super alloc] initListener]; });
  return listener;
}

+ (void)addObserver:(TObserver)observer
{
  dispatch_async(dispatch_get_main_queue(), ^
  {
    MWMFrameworkListener * listener = [MWMFrameworkListener listener];
    if ([observer conformsToProtocol:pRouteBuildingObserver])
      [listener.routeBuildingObservers addObject:observer];
    if ([observer conformsToProtocol:pMyPositionObserver])
      [listener.myPositionObservers addObject:observer];
    if ([observer conformsToProtocol:pStorageObserver])
      [listener.storageObservers addObject:observer];
    if ([observer conformsToProtocol:pDrapeObserver])
      [listener.drapeObservers addObject:observer];
  });
}

+ (void)removeObserver:(TObserver)observer
{
  dispatch_async(dispatch_get_main_queue(), ^
  {
    MWMFrameworkListener * listener = [MWMFrameworkListener listener];
    [listener.routeBuildingObservers removeObject:observer];
    [listener.myPositionObservers removeObject:observer];
    [listener.storageObservers removeObject:observer];
    [listener.drapeObservers removeObject:observer];
  });
}

- (instancetype)initListener
{
  self = [super init];
  if (self)
  {
    _routeBuildingObservers = [TObservers weakObjectsHashTable];
    _myPositionObservers = [TObservers weakObjectsHashTable];
    _storageObservers = [TObservers weakObjectsHashTable];
    _drapeObservers = [TObservers weakObjectsHashTable];

    [self registerRouteBuilderListener];
    [self registerMyPositionListener];
    [self registerStorageObserver];
    [self registerDrapeObserver];
  }
  return self;
}

#pragma mark - MWMFrameworkRouteBuildingObserver

- (void)registerRouteBuilderListener
{
  using namespace routing;
  using namespace storage;
  TObservers * observers = self.routeBuildingObservers;
  auto & f = GetFramework();
  // TODO(ldragunov,rokuz): Thise two routing callbacks are the only framework callbacks which does not guarantee
  // that they are called on a main UI thread context. Discuss it with Lev.
  // Simplest solution is to insert RunOnGuiThread() call in the core where callbacks are called.
  // This will help to avoid unnecessary parameters copying and will make all our framework callbacks
  // consistent: every notification to UI will run on a main UI thread.
  f.SetRouteBuildingListener([observers](IRouter::ResultCode code, TCountriesVec const & absentCountries)
  {
    loopWrappers(observers, [code, absentCountries](TRouteBuildingObserver observer)
    {
      [observer processRouteBuilderEvent:code countries:absentCountries];
    });
  });
  f.SetRouteProgressListener([observers](float progress)
  {
    loopWrappers(observers, [progress](TRouteBuildingObserver observer)
    {
      if ([observer respondsToSelector:@selector(processRouteBuilderProgress:)])
        [observer processRouteBuilderProgress:progress];
    });
  });
}

#pragma mark - MWMFrameworkMyPositionObserver

- (void)registerMyPositionListener
{
  TObservers * observers = self.myPositionObservers;
  auto & f = GetFramework();
  f.SetMyPositionModeListener([self, observers](location::EMyPositionMode mode)
  {
    self.myPositionMode = mode;
    loopWrappers(observers, [mode](TMyPositionObserver observer)
    {
      [observer processMyPositionStateModeEvent:mode];
    });
  });
}

#pragma mark - MWMFrameworkStorageObserver

- (void)registerStorageObserver
{
  TObservers * observers = self.storageObservers;
  auto & s = GetFramework().Storage();
  s.Subscribe([observers](TCountryId const & countryId)
  {
    for (TStorageObserver observer in observers)
      [observer processCountryEvent:countryId];
  },
  [observers](TCountryId const & countryId, TLocalAndRemoteSize const & progress)
  {
    for (TStorageObserver observer in observers)
    {
      if ([observer respondsToSelector:@selector(processCountry:progress:)])
        [observer processCountry:countryId progress:progress];
    }
  });
}

#pragma mark - MWMFrameworkDrapeObserver

- (void)registerDrapeObserver
{
  TObservers * observers = self.drapeObservers;
  auto & f = GetFramework();
  f.SetCurrentCountryChangedListener([observers](TCountryId const & countryId)
  {
    for (TDrapeObserver observer in observers)
      [observer processViewportCountryEvent:countryId];
  });
}

@end
