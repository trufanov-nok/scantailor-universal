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

#ifndef ACCELERATION_PLUGIN_H_
#define ACCELERATION_PLUGIN_H_

#include "acceleration_config.h"
#include "AcceleratableOperations.h"
#include <string>
#include <vector>
#include <string>
#include <memory>
#include <QtPlugin>

#define AccelerationPlugin_iid "org.scantailor.AccelerationPlugin"

/**
 * @note This interface is not thread-safe.
 */
class ACCELERATION_EXPORT AccelerationPlugin
{
public:
	virtual ~AccelerationPlugin() {}

	/**
	 * @brief Returns a list of human-readable device names.
	 */
	virtual std::vector<std::string> devices() const = 0;

	/**
	 * @brief Selects the device to be used for acceleration.
	 *
	 * The selection shall be persistent through a mechanism
	 * such as QSettings.
	 *
	 * @param device_name Device name as returned by devices().
	 *        An attempt to select an unknown device will be
	 *        silently ignored.
	 */
	virtual void selectDevice(std::string const& device_name) = 0;

	/**
	 * @brief Returns the explicitly or implicitly selected device.
	 *
	 * In the absence of selectDevice() calls and with missing or invalid
	 * persistent settings, preference, the first available device is
	 * considered to be selected and will be returned from this function.
	 * When no devices are available, an empty string is returned.
	 */
	virtual std::string selectedDevice() const = 0;

	/**
	 * @brief Construct or re-use an instance of AcceleratableOperations.
	 *
	 * In case of a problem, the fallback is returned.
	 */
	virtual std::shared_ptr<AcceleratableOperations> getOperations(
		std::shared_ptr<AcceleratableOperations> const& fallback) = 0;

	/**
	 * @brief Release any resources early, rather than waiting for this
	 *        object to be destroyed.
	 *
	 * Clients shall not call any other methods of this interface after
	 * calling this method.
	 *
	 * @see DefaultAccelerationProvider::releaseResources()
	 */
	virtual void releaseResources() = 0;
};

Q_DECLARE_INTERFACE(AccelerationPlugin, AccelerationPlugin_iid)

#endif
