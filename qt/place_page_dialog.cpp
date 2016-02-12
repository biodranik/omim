#include "qt/place_page_dialog.hpp"

#include "map/place_page_info.hpp"

#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

PlacePageDialog::PlacePageDialog(QWidget * parent, place_page::Info const & info)
  : QDialog(parent)
{
  QGridLayout * grid = new QGridLayout();
  int row = 0;
  {
    grid->addWidget(new QLabel("lat lon"), row, 0);
    ms::LatLon const ll = info.GetLatLon();
    string const llstr = strings::to_string_dac(ll.lat, 7) + " " + strings::to_string_dac(ll.lon, 7);
    QLabel * coordsLabel = new QLabel(llstr.c_str());
    coordsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    grid->addWidget(coordsLabel, row++, 1);
  }
  if (!info.GetTitle().empty())
  {
    grid->addWidget(new QLabel("name"), row, 0);
    QLabel * nameLabel = new QLabel(info.GetTitle().c_str());
    nameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    grid->addWidget(nameLabel, row++, 1);
  }
  if (!info.GetSubtitle().empty())
  {
    grid->addWidget(new QLabel("subtitle"), row, 0);
    QLabel * subtitleLabel = new QLabel(info.GetSubtitle().c_str());
    subtitleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    grid->addWidget(subtitleLabel, row++, 1);
  }
  // Display feature types only if they are different from subtitle.
  if (info.IsFeature() && DebugPrint(info.GetTypes()) != info.GetSubtitle())
  {
    grid->addWidget(new QLabel("types"), row, 0);
    grid->addWidget(new QLabel(DebugPrint(info.GetTypes()).c_str()), row++, 1);
  }

  for (auto const metaType : info.GetMetadata().GetPresentTypes())
  {
    grid->addWidget(new QLabel(DebugPrint(static_cast<feature::Metadata::EType>(metaType)).c_str()), row, 0);
    grid->addWidget(new QLabel(info.GetMetadata().Get(metaType).c_str()), row++, 1);
  }

  QDialogButtonBox * dbb = new QDialogButtonBox();
  QPushButton * closeButton = new QPushButton("Close");
  closeButton->setDefault(true);
  connect(closeButton, SIGNAL(clicked()), this, SLOT(OnClose()));
  dbb->addButton(closeButton, QDialogButtonBox::RejectRole);

  if (info.IsEditable())
  {
    QPushButton * editButton = new QPushButton("Edit Place");
    connect(editButton, SIGNAL(clicked()), this, SLOT(OnEdit()));
    dbb->addButton(editButton, QDialogButtonBox::AcceptRole);
  }
  grid->addWidget(dbb);
  setLayout(grid);

  string const title = string("Place Page") + (info.IsBookmark() ? " (bookmarked)" : "");
  setWindowTitle(title.c_str());
}

void PlacePageDialog::OnClose()
{
  reject();
}

void PlacePageDialog::OnEdit()
{
  accept();
}
