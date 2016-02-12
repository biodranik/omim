#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_meta.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "std/vector.hpp"

class FeatureType;

namespace osm
{

/// Holds information to construct editor's UI.
struct EditableProperties
{
  bool m_name = false;
  /// If true, enables editing of house number, street address and post code.
  bool m_address = false;
  vector<feature::Metadata::EType> m_metadata;
  bool IsEditable() const { return m_name || m_address || !m_metadata.empty(); }
};

struct LocalizedName
{
  // m_code, m_lang and m_langName are defined in StringUtf8Multilang.
  int8_t m_code;
  char const * m_lang;
  char const * m_langName;
  string m_name;
};

class EditableFeature
{
public:
  bool IsNameEditable() const { return m_editableProperties.m_name; }
  bool IsAddressEditable() const { return m_editableProperties.m_address; }
  vector<feature::Metadata::EType> const & GetEditableFields() const { return m_editableProperties.m_metadata; }
  void SetEditableProperties(osm::EditableProperties const & props) { m_editableProperties = props; }

  FeatureID const & GetFeatureID() const { return m_featureID; }
  void SetFeatureID(FeatureID const & fid) { m_featureID = fid; }

  string GetDefaultName() const;
  vector<LocalizedName> GetLocalizedName() const;
  StringUtf8Multilang const & GetName() const { return m_name; }
  void SetName(StringUtf8Multilang const & name) { m_name = name; }

  ms::LatLon GetLatLon() const { return MercatorBounds::ToLatLon(m_mercator); }
  m2::PointD const & GetMercator() const { return m_mercator; }
  void SetMercator(m2::PointD const & center) { m_mercator = center; }

  /// @returns "the best" type for edited feature to display in UI.
  string GetLocalizedType() const;
  feature::TypesHolder const & GetTypes() const { return m_types; }
  void SetTypes(feature::TypesHolder const & types) { m_types = types; }

  string const & GetStreet() const { return m_street; }
  void SetStreet(string const & street) { m_street = street; }

  vector<string> const & GetNearbyStreets() const { return m_nearbyStreets; }
  void SetNearbyStreets(vector<string> const & streets) { m_nearbyStreets = streets; }

  string const & GetHouseNumber() const { return m_houseNumber; }
  void SetHouseNumber(string const & houseNumber) { m_houseNumber = houseNumber; }

  feature::Metadata const & GetMetadata() const { return m_metadata; }
  feature::Metadata & GetMetadata() { return m_metadata; }
  void SetMetadata(feature::Metadata const & metadata) { m_metadata = metadata; }

private:
  m2::PointD m_mercator;
  StringUtf8Multilang m_name;
  feature::TypesHolder m_types;
  feature::Metadata m_metadata;
  string m_houseNumber;
  string m_street;
  vector<string> m_nearbyStreets;
  EditableProperties m_editableProperties;

  FeatureID m_featureID;
};
}  // namespace osm
