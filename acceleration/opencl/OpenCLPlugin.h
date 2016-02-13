/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015-2016  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef OPENCL_PLUGIN_H_
#define OPENCL_PLUGIN_H_

#include "AccelerationPlugin.h"
#include "AcceleratableOperations.h"
#include <CL/cl2.hpp>
#include <QObject>
#include <vector>
#include <string>
#include <memory>
#include <cstddef>

namespace opencl
{

class OpenCLPlugin : public QObject, public AccelerationPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID AccelerationPlugin_iid)
	Q_INTERFACES(AccelerationPlugin)
public:
	OpenCLPlugin();

	virtual ~OpenCLPlugin();

	virtual std::vector<std::string> devices() const;

	virtual void selectDevice(std::string const& device_name);

	virtual std::string selectedDevice() const;

	virtual std::shared_ptr<AcceleratableOperations> getOperations(
		std::shared_ptr<AcceleratableOperations> const& fallback);

	virtual void releaseResources();
private:
	std::vector<cl::Device> m_devices;
	cl::Device m_selectedDevice;
	std::shared_ptr<AcceleratableOperations> m_ptrCachedOps;
};

} // namespace opencl

#endif
