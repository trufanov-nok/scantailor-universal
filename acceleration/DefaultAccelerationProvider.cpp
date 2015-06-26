/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "DefaultAccelerationProvider.h"
#include "NonAcceleratedOperations.h"
#include "AccelerationPlugin.h"
#include <QCoreApplication>
#include <QPluginLoader>
#include <QSettings>
#include <QDebug>
#include "config.h"

DefaultAccelerationProvider::DefaultAccelerationProvider(QObject* parent)
:	QObject(parent)
,	m_pPlugin(nullptr)
,	m_ptrNonAcceleratedOperations(std::make_shared<NonAcceleratedOperations>())
{
	processUpdatedConfiguration();
}

void
DefaultAccelerationProvider::processUpdatedConfiguration()
{
	QSettings settings;

#ifdef ENABLE_OPENCL
	if (settings.value("settings/enable_opencl", false).toBool()) {
		QPluginLoader loader("opencl_plugin");
		if (loader.load()) {
			m_pPlugin = qobject_cast<AccelerationPlugin*>(loader.instance());
		} else {
			qDebug() << "OpenCL plugin failed to load: " << loader.errorString();
		}
	} else {
		m_pPlugin = nullptr;
		// It will be kept in memory till unload() is called on QPluginLoader.
		// That happens automatically by static destruction process, though we
		// do that explicitly in main().
	}
#endif
}

void
DefaultAccelerationProvider::releaseResources()
{
#ifdef ENABLE_OPENCL
	// We could have just called unload() on QPluginLoader, but
	// unfortunately Qt's reference counting on plugins prevents
	// that from working.
	AccelerationPlugin* opencl_plugin = m_pPlugin;
	if (!opencl_plugin) {
		QPluginLoader loader("opencl_plugin");
		if (loader.isLoaded()) {
			opencl_plugin = qobject_cast<AccelerationPlugin*>(loader.instance());
		}
	}
	if (opencl_plugin) {
		opencl_plugin->releaseResources();
	}
#endif
}

std::shared_ptr<AcceleratableOperations>
DefaultAccelerationProvider::getOperations()
{
	if (m_pPlugin) {
		return m_pPlugin->getOperations(m_ptrNonAcceleratedOperations);
	} else {
		return m_ptrNonAcceleratedOperations;
	}
}
