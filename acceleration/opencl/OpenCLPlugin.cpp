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

// Suppress a warning about sscanf()
#define _CRT_SECURE_NO_WARNINGS

#include "OpenCLPlugin.h"
#include "OpenCLAcceleratedOperations.h"
#include <QSettings>
#include <QVariant>
#include <QByteArray>
#include <QDebug>
#include <QtGlobal>
#include <QSysInfo>
#include <exception>
#include <cstdio>
#include <utility>

static void initQtResources()
{
	// Q_INIT_RESOURCE has to be invoked from global namespace.
	Q_INIT_RESOURCE(resources);
}

static void cleanupQtResources()
{
	// Q_CLEANUP_RESOURCE has to be invoked from global namespace.
	Q_CLEANUP_RESOURCE(resources);
}

namespace opencl
{

OpenCLPlugin::OpenCLPlugin()
{
	initQtResources();

	QByteArray const selected_device(
		QSettings().value("opencl/device", QByteArray()).toByteArray()
	);
	std::string const selected_device_str(selected_device.data(), selected_device.size());

	try {
		std::vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);
		for (cl::Platform const& platform : platforms) {
			std::vector<cl::Device> devices;
			platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

			for (auto const& device : devices) {
				if (!device.getInfo<CL_DEVICE_AVAILABLE>()) {
					continue;
				}
				if (!device.getInfo<CL_DEVICE_COMPILER_AVAILABLE>()) {
					continue;
				}
				if (!device.getInfo<CL_DEVICE_IMAGE_SUPPORT>()) {
					continue;
				}
				if (bool(device.getInfo<CL_DEVICE_ENDIAN_LITTLE>()) !=
						bool(QSysInfo::ByteOrder == QSysInfo::LittleEndian)) {
					// If the endiannes differs, we would have to convert the buffers
					// we pass, which we don't currently do.
					continue;
				}

				std::string const opencl_version = device.getInfo<CL_DEVICE_VERSION>();
				int major_version = 0;
				int minor_version = 0;
				sscanf(opencl_version.c_str(), "OpenCL %d.%d", &major_version, &minor_version);
				if (std::make_pair(major_version, minor_version) < std::make_pair(1, 1)) {
					// Doesn't support OpenCL 1.1
					continue;
				}

				m_devices.push_back(device);
				if (device.getInfo<CL_DEVICE_NAME>() == selected_device_str) {
					m_selectedDevice = device;
				}
			}
		}

		// Select the first device, if none was selected.
		if (!m_selectedDevice() && !m_devices.empty()) {
			m_selectedDevice = m_devices.front();
		}
	} catch (std::exception const& e) {
		qDebug() << "OpenCL error: " << e.what();
	}
}

OpenCLPlugin::~OpenCLPlugin()
{
	cleanupQtResources();
}

std::vector<std::string>
OpenCLPlugin::devices() const
{
	std::vector<std::string> names;
	names.reserve(m_devices.size());

	for (auto const& device : m_devices) {
		names.push_back(device.getInfo<CL_DEVICE_NAME>());
	}

	return names;
}

void
OpenCLPlugin::selectDevice(std::string const& device_name)
{
	QSettings().setValue(
		"opencl/device",
		QByteArray(device_name.c_str(), device_name.size())
	);

	for (auto const& device : m_devices) {
		if (device.getInfo<CL_DEVICE_NAME>() == device_name) {
			m_selectedDevice = device;
			m_ptrCachedOps.reset();
		}
	}
}

std::string
OpenCLPlugin::selectedDevice() const
{
	if (m_selectedDevice()) {
		return m_selectedDevice.getInfo<CL_DEVICE_NAME>();
	} else {
		return std::string();
	}
}

std::shared_ptr<AcceleratableOperations>
OpenCLPlugin::getOperations(
	std::shared_ptr<AcceleratableOperations> const& fallback)
{
	if (!m_selectedDevice()) {
		return fallback;
	}

	if (m_ptrCachedOps) {
		return m_ptrCachedOps;
	}

	try {
		qDebug() << "Selected OpenCL device: " << m_selectedDevice.getInfo<CL_DEVICE_NAME>().c_str();
		cl::Context context(std::vector<cl::Device>(1, m_selectedDevice));
		m_ptrCachedOps = std::make_shared<OpenCLAcceleratedOperations>(context, fallback);
		return m_ptrCachedOps;
	} catch (std::exception const& e) {
		qDebug() << "OpenCL error: " << e.what();
		return fallback;
	}
}

void
OpenCLPlugin::releaseResources()
{
	m_ptrCachedOps.reset();
	m_devices.clear();
	m_selectedDevice = cl::Device();
}

} // namespace opencl
