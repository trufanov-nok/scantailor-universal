#include "ApplyToDialog.h"
#include "ui/ui_ApplyToDialog.h"

ApplyToDialog::ApplyToDialog(QWidget* parent, const PageId& cur_page, const PageSelectionAccessor& page_selection_accessor, const PageView viewType) :
    QDialog(parent),
    ui(new Ui::ApplyToDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    ui->widgetPageRangeSelector->setData(cur_page, page_selection_accessor, viewType);
    // clear whatever on panels in UI designer
    initNewLeftSettingsPanel();
    initNewTopSettingsPanel();
}

ApplyToDialog::~ApplyToDialog()
{
    delete ui;
}

void clearLayout(QLayout& l)
{
    while (l.count() > 0) {
        l.removeItem(l.itemAt(0));
    }
}

QLayout& ApplyToDialog::initNewLeftSettingsPanel()
{
    QLayout* l = ui->widgetSettingsLeft->layout();
    if (l) {
        clearLayout(*l);
    }

    return getLeftSettingsPanel();
}

QLayout& ApplyToDialog::getLeftSettingsPanel() const
{
    return * ui->widgetSettingsLeft->layout();
}

QLayout& ApplyToDialog::initNewTopSettingsPanel()
{
    QLayout* l = ui->widgetSettingsTop->layout();
    if (l) {
        clearLayout(*l);
    }

    return getTopSettingsPanel();
}

QLayout& ApplyToDialog::getTopSettingsPanel() const
{
    return * ui->widgetSettingsTop->layout();
}

void ApplyToDialog::showEvent(QShowEvent* /*event*/)
{
    QLayout* l = ui->widgetSettingsLeft->layout();
    const bool left_settings_visible = l && l->count() > 0;
    ui->widgetSettingsLeft->setVisible(left_settings_visible);
    ui->lineV->setVisible(left_settings_visible);

    l = ui->widgetSettingsTop->layout();
    const bool top_settings_visible = l && l->count() > 0;
    ui->widgetSettingsTop->setVisible(top_settings_visible);
}

PageRangeSelectorWidget& ApplyToDialog::getPageRangeSelectorWidget() const
{
    return * ui->widgetPageRangeSelector;
}

void ApplyToDialog::accept()
{
    for (Validator* v : qAsConst(m_Validators)) {
        if (!v->validate()) {
            return;
        }
    }

    done(Accepted);
}
