#ifndef ALIGNMENTWIDGET_H
#define ALIGNMENTWIDGET_H

#include "Alignment.h"
#include <QWidget>
#include <QComboBox>
#include <QToolButton>
#include <QMenu>

namespace Ui {
class AlignmentWidget;
}

using namespace page_layout;

class AlignmentComboBox: public QComboBox
{
    // ComboBox that displays only icons till popup
    // required bcs it looks too bad on Win
  public:
    AlignmentComboBox(QWidget *parent = Q_NULLPTR);
    void showPopup();
    void hidePopup();
private:
    void updContextMenu();
private:
    QStringList m_itemsText;
    const QString m_empty;
    QMenu m_menu;
};

class AlignmentWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AlignmentWidget(QWidget *parent = 0, Alignment* alignment = 0);
    ~AlignmentWidget();

    void setAlignment(Alignment* alignment);
    void setUseAutoMagnetAlignment(bool val);
    void setUseOriginalProportionsAlignment(bool val);

    Alignment* alignment() const { return m_alignment; }
    bool useAutoMagnetAlignment() const { return m_useAutoMagnetAlignment; }
    bool useOriginalProportionsAlignment() const { return m_useOriginalProportionsAlignment; }

    void displayAlignment();

private:
    typedef std::map<QToolButton*, Alignment> AlignmentByButton;
    typedef AlignmentByButton::value_type KeyVal;


    void markAlignmentButtons();
    void resetAdvAlignmentComboBoxes(QComboBox* cb, const int composite_alignment);
    void clickAlignmentButton();
    void updateAlignmentButtonsDisplay();

    Ui::AlignmentWidget *ui;
    Alignment* m_alignment;
    bool m_useAutoMagnetAlignment;
    bool m_useOriginalProportionsAlignment;
    int m_advancedAlignment;
    AlignmentByButton m_alignmentByButton;

signals:
    void alignmentChanged();

private slots:
    void alignmentButtonClicked(bool checked);
    void on_alignWithOthersCB_toggled(bool checked);
    void on_cbAutoMagnet_currentIndexChanged(int index);
    void on_cbOriginalProp_currentIndexChanged(int index);
    void on_btnResetAdvAlignment_clicked();
};


#endif // ALIGNMENTWIDGET_H
