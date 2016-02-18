#pragma once

#include <QtWidgets/QDialog>

namespace osm
{
class EditableFeature;
}  // namespace osm

class EditorDialog : public QDialog
{
  Q_OBJECT
public:
  EditorDialog(QWidget * parent, osm::EditableFeature & ef);
private slots:
  void OnSave();
private:
  osm::EditableFeature & m_editableFeature;
};
