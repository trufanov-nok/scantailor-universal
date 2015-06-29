/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SettingsDialog.h"
#include "OpenGLSupport.h"
#include "acceleration/AccelerationPlugin.h"
#include "config.h"
#include <QDebug>
#include <QPluginLoader>
#include <QSettings>
#include <QVariant>
#include <QByteArray>
#include <QString>
#include <string>

SettingsDialog::SettingsDialog(QWidget* parent)
:	QDialog(parent)
,	m_pOpenCLPlugin(nullptr)
{
	ui.setupUi(this);
	QString const openglDevicePattern = ui.openglDeviceLabel->text();

	QSettings settings;

#ifndef ENABLE_OPENGL
	ui.enableOpenglCb->setChecked(false);
	ui.enableOpenglCb->setEnabled(false);
	ui.openglDeviceLabel->setEnabled(false);
	ui.openglDeviceLabel->setText(tr("Built without OpenGL support"));
#else
	if (!OpenGLSupport::supported()) {
		ui.enableOpenglCb->setChecked(false);
		ui.enableOpenglCb->setEnabled(false);
		ui.openglDeviceLabel->setEnabled(false);
		ui.openglDeviceLabel->setText(tr("Your hardware / driver don't provide the necessary features"));
	} else {
		ui.enableOpenglCb->setChecked(
			settings.value("settings/enable_opengl", false).toBool()
		);
		ui.openglDeviceLabel->setText(openglDevicePattern.arg(OpenGLSupport::deviceName()));
	}
#endif

#ifndef ENABLE_OPENCL
	ui.enableOpenclCb->setChecked(false);
	ui.enableOpenclCb->setEnabled(false);
	ui.openclDeviceCombo->setEnabled(false);
	ui.openclDeviceCombo->addItem(tr("Built without OpenCL support"));
#else
	{
		QPluginLoader loader("opencl_plugin");
		if (loader.load()) {
			m_pOpenCLPlugin = qobject_cast<AccelerationPlugin*>(loader.instance());
		} else {
			qDebug() << "OpenCL plugin failed to load: " << loader.errorString();
		}
		if (!m_pOpenCLPlugin) {
			ui.enableOpenclCb->setChecked(false);
			ui.enableOpenclCb->setEnabled(false);
			ui.openclDeviceCombo->setEnabled(false);
			ui.openclDeviceCombo->addItem(tr("Failed to initialize OpenCL"));
		} else {
			ui.enableOpenclCb->setChecked(
				settings.value("settings/enable_opencl", false).toBool()
			);

			std::string const selected_device = m_pOpenCLPlugin->selectedDevice();
			for (std::string const& device : m_pOpenCLPlugin->devices()) {
				ui.openclDeviceCombo->addItem(
					QString::fromStdString(device), QByteArray(device.c_str(), device.size())
				);
				if (device == selected_device) {
					ui.openclDeviceCombo->setCurrentIndex(ui.openclDeviceCombo->count() - 1);
				}
			}

			if (ui.openclDeviceCombo->count() == 0) {
				ui.enableOpenclCb->setChecked(false);
				ui.enableOpenclCb->setEnabled(false);
				ui.openclDeviceCombo->setEnabled(false);
				ui.openclDeviceCombo->addItem(tr("No OpenCL-capable devices found"));
			}
		}
	}
#endif

	connect(ui.buttonBox, SIGNAL(accepted()), SLOT(commitChanges()));
}

SettingsDialog::~SettingsDialog()
{
}

void
SettingsDialog::commitChanges()
{
	{
		QSettings settings;

#ifdef ENABLE_OPENGL
		settings.setValue("settings/enable_opengl", ui.enableOpenglCb->isChecked());
#endif

#ifdef ENABLE_OPENCL
		settings.setValue("settings/enable_opencl", ui.enableOpenclCb->isChecked());
		if (m_pOpenCLPlugin) {
			QByteArray const device = ui.openclDeviceCombo->currentData().toByteArray();
			m_pOpenCLPlugin->selectDevice(std::string(device.data(), device.size()));
		}
#endif
	}

	emit settingsUpdated();
}
