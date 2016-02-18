#include "qt/editor_dialog.hpp"

#include "indexer/editable_feature.hpp"

#include "base/string_utils.hpp"

#include "std/algorithm.hpp"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

#include <QtCore/QSignalMapper>

constexpr char const * kStreetObjectName = "addr:street";
constexpr char const * kHouseNumberObjectName = "addr:housenumber";

EditorDialog::EditorDialog(QWidget * parent, osm::EditableFeature & ef) : QDialog(parent), m_editableFeature(ef)
{
  QGridLayout * grid = new QGridLayout();
  int row = 0;

  { // Coordinates.
    ms::LatLon const ll = ef.GetLatLon();
    grid->addWidget(new QLabel("Latitude/Longitude:"), row, 0);
    QHBoxLayout * coords = new QHBoxLayout();
    coords->addWidget(new QLabel(QString::fromStdString(strings::to_string_dac(ll.lat, 7) + " " +
                                                        strings::to_string_dac(ll.lon, 7))));
    grid->addLayout(coords, row++, 1);
  }

  { // Feature types.
    grid->addWidget(new QLabel("Localized Type:"), row, 0);
    grid->addWidget(new QLabel(QString::fromStdString(ef.GetLocalizedType())), row++, 1);

    grid->addWidget(new QLabel("Raw Types:"), row, 0);
    grid->addWidget(new QLabel(QString::fromStdString(DebugPrint(ef.GetTypes()))), row++, 1);
  }

  if (ef.IsNameEditable())
  { // Names.
    char const * defaultLangStr = StringUtf8Multilang::GetLangByCode(StringUtf8Multilang::kDefaultCode);
    // Default name editor is always displayed, even if feature name is empty.
    grid->addWidget(new QLabel(QString("Name:")), row, 0);
    QLineEdit * defaultName = new QLineEdit();
    defaultName->setObjectName(defaultLangStr);
    QGridLayout * namesGrid = new QGridLayout();
    int namesRow = 0;
    namesGrid->addWidget(defaultName, namesRow++, 0, 1, 0);

    for (osm::LocalizedName const & ln : ef.GetLocalizedName())
    {
      if (ln.m_code == StringUtf8Multilang::kDefaultCode)
      {
        defaultName->setText(QString::fromStdString(ln.m_name));
      }
      else
      {
        char const * langStr = StringUtf8Multilang::GetLangByCode(ln.m_code);
        namesGrid->addWidget(new QLabel(ln.m_lang), namesRow, 0);
        QLineEdit * lineEditName = new QLineEdit(QString::fromStdString(ln.m_name));
        lineEditName->setReadOnly(!ef.IsNameEditable());
        lineEditName->setObjectName(langStr);
        namesGrid->addWidget(lineEditName, namesRow++, 1);
      }
    }
    grid->addLayout(namesGrid, row++, 1);
  }

  if (ef.IsAddressEditable())
  { // Address rows.
    vector<string> nearbyStreets = ef.GetNearbyStreets();
    // If feature does not have a specified street, display empty combo box.
    if (ef.GetStreet().empty())
      nearbyStreets.insert(nearbyStreets.begin(), "");
    grid->addWidget(new QLabel(QString(kStreetObjectName)), row, 0);
    QComboBox * cmb = new QComboBox();
    for (auto const & street : nearbyStreets)
      cmb->addItem(street.c_str());
    cmb->setObjectName(kStreetObjectName);
    grid->addWidget(cmb, row++, 1);

    grid->addWidget(new QLabel(QString(kHouseNumberObjectName)), row, 0);
    QLineEdit * houseLineEdit = new QLineEdit();
    houseLineEdit->setText(ef.GetHouseNumber().c_str());
    houseLineEdit->setObjectName(kHouseNumberObjectName);
    grid->addWidget(houseLineEdit, row++, 1);
  }

  { // Only editable metadata rows.
    for (auto const field : ef.GetEditableFields())
    {
      feature::Metadata::EType const cfield = static_cast<feature::Metadata::EType>(field);
      QString const fieldName = QString::fromStdString(DebugPrint(cfield));
      grid->addWidget(new QLabel(fieldName), row, 0);
      QLineEdit * lineEdit = new QLineEdit(QString::fromStdString(ef.GetMetadata().Get(cfield)));
      // Mark line editor to query it's text value when editing is finished.
      lineEdit->setObjectName(fieldName);
      grid->addWidget(lineEdit, row++, 1);
    }
  }

  { // Dialog buttons.
    QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(OnSave()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    // Delete button should send custom int return value from dialog.
    QPushButton * deletePOIButton = new QPushButton("Delete POI");
    QSignalMapper * signalMapper = new QSignalMapper();
    connect(deletePOIButton, SIGNAL(clicked()), signalMapper, SLOT(map()));
    signalMapper->setMapping(deletePOIButton, QDialogButtonBox::DestructiveRole);
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(done(int)));
    buttonBox->addButton(deletePOIButton, QDialogButtonBox::DestructiveRole);
    grid->addWidget(buttonBox, row++, 1);
  }

  setLayout(grid);
  setWindowTitle("OSM Editor");
}

void EditorDialog::OnSave()
{
  // Store all edits.
  if (m_editableFeature.IsNameEditable())
  {
    StringUtf8Multilang names;
    for (int8_t langCode = StringUtf8Multilang::kDefaultCode; langCode < StringUtf8Multilang::kMaxSupportedLanguages; ++langCode)
    {
      QLineEdit * le = findChild<QLineEdit *>(StringUtf8Multilang::GetLangByCode(langCode));
      if (!le)
        continue;
      string const name = le->text().toStdString();
      if (!name.empty())
        names.AddString(langCode, name);
    }
    m_editableFeature.SetName(names);
  }
  if (m_editableFeature.IsAddressEditable())
  {
    m_editableFeature.SetHouseNumber(findChild<QLineEdit *>(kHouseNumberObjectName)->text().toStdString());
    m_editableFeature.SetStreet(findChild<QComboBox *>()->currentText().toStdString());
  }
  for (feature::Metadata::EType mdType : m_editableFeature.GetEditableFields())
  {
    QLineEdit * editor = findChild<QLineEdit *>(QString::fromStdString(DebugPrint(mdType)));
    if (editor)
      m_editableFeature.GetMetadata().Set(mdType, editor->text().toStdString());
  }

  accept();
}
