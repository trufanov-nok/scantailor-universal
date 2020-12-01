#ifndef APPLYTODIALOG_H
#define APPLYTODIALOG_H

#include <QDialog>
#include "PageRangeSelectorWidget.h"

namespace Ui
{
class ApplyToDialog;
}

class ApplyToDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ApplyToDialog(QWidget* parent, PageId const& cur_page,
                           PageSelectionAccessor const& page_selection_accessor,
                           const PageView viewType = PageView::PAGE_VIEW);
    ~ApplyToDialog();

    QLayout& initNewLeftSettingsPanel();

    QLayout& initNewTopSettingsPanel();

    QLayout& getLeftSettingsPanel() const;

    QLayout& getTopSettingsPanel() const;

    PageRangeSelectorWidget& getPageRangeSelectorWidget() const;

    void showEvent(QShowEvent*) override;

    class Validator
    {
    public:
        virtual bool validate() = 0;
    };

    void registerValidator(Validator* v)
    {
        if (v) {
            m_Validators.append(v);
        }
    }

public slots:
    void accept() override;

private:
    Ui::ApplyToDialog* ui;
    QVector<Validator*> m_Validators;
};

#endif // APPLYTODIALOG_H
