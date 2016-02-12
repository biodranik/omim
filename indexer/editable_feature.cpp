#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/editable_feature.hpp"

#include "platform/preferred_languages.hpp"

#include "base/macros.hpp"

namespace osm
{
string EditableFeature::GetDefaultName() const
{
  string name;
  UNUSED_VALUE(m_name.GetString(StringUtf8Multilang::kDefaultCode, name));
  return name;
}

vector<LocalizedName> EditableFeature::GetLocalizedName() const
{
  vector<LocalizedName> result;
  m_name.ForEach([&result](int8_t code, string const & name) -> bool
  {
    result.push_back({code, StringUtf8Multilang::GetLangByCode(code),
                     StringUtf8Multilang::GetLangNameByCode(code), name});
    return true;
  });
  return result;
}

string EditableFeature::GetLocalizedType() const
{
  ASSERT(!m_types.Empty(), ());
  feature::TypesHolder copy(m_types);
  copy.SortBySpec();
  CategoriesHolder const & categories = GetDefaultCategories();
  return categories.GetReadableFeatureType(*copy.begin(), categories.MapLocaleToInteger(languages::GetCurrentOrig()));
}
}  // namespace osm
