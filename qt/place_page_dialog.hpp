#pragma once

#include <QtWidgets/QDialog>

namespace place_page
{
struct Info;
}

class PlacePageDialog : public QDialog
{
  Q_OBJECT
public:
  PlacePageDialog(QWidget * parent, place_page::Info const & info);

private slots:
  void OnClose();
  void OnEdit();
};
