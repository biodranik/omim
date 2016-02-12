#pragma once

#include "map/bookmark.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/feature_meta.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "std/string.hpp"

namespace place_page
{
struct Info
{
  bool IsFeature() const { return m_featureID.IsValid(); }
  bool IsBookmark() const { return m_bac != MakeEmptyBookmarkAndCategory(); }
  bool IsMyPosition() const { return m_isMyPosition; }
  /// @returns true if Back API button should be displayed.
  bool HasApiUrl() const { return !m_apiUrl.empty(); }
  /// @returns true if Edit Place button should be displayed.
  bool IsEditable() const { return m_isEditable; }

//  /// Main Place Page title.
//  string GetTitle() const;
//  AddressInfo GetAddress() const;

  string GetTitle() const { return m_title; }
  /// Types, cuisines, stars, elevation etc.
  string GetSubtitle() const { return m_subtitle; }
  m2::PointD const & GetMercator() const { return m_mercator; }
  ms::LatLon GetLatLon() const { return MercatorBounds::ToLatLon(m_mercator); }
  feature::TypesHolder const & GetTypes() const { return m_types; }
  feature::Metadata const & GetMetadata() const { return m_metadata; }
  string const & GetApiUrl() const { return m_apiUrl; }

  string m_title;
  string m_subtitle;
  m2::PointD m_mercator;

  FeatureID m_featureID;
  // TODO(AlexZ): Use string types representation? Reuse code from search::AdrressInfo?
  feature::TypesHolder m_types;
  feature::Metadata m_metadata;

  BookmarkAndCategory m_bac = MakeEmptyBookmarkAndCategory();

  /// [Deep] link to open when "Back" button is pressed in a Place Page.
  string m_apiUrl;

  bool m_isMyPosition = false;
  bool m_isEditable = false;
};
}  // namespace place_page
