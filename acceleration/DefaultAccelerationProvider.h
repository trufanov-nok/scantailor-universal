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

#ifndef DEFAULT_ACCELERATION_PROVIDER_H_
#define DEFAULT_ACCELERATION_PROVIDER_H_

#include "acceleration_config.h"
#include "AcceleratableOperations.h"
#include <memory>
#include <QObject>

class AccelerationPlugin;

/**
 * @brief Provides access to accelerated (think OpenCL) operations.
 *
 * @note This class is not thread-safe.
 */
class ACCELERATION_EXPORT DefaultAccelerationProvider : public QObject
{
	Q_OBJECT
public:
	/**
	 * @brief Initializes the acceleration engine, loading plugins if necessary.
	 */
	DefaultAccelerationProvider(QObject* parent = nullptr);

	/**
	 * @brief Re-checks configuration and loads plug-ins if necessary.
	 *
	 * This method should be called after changing the value of "settings/enable_opencl"
	 * key in QSettings. This method is called from this class' constructor.
	 */
	void processUpdatedConfiguration();

	/**
	 * @brief Calls AccelerationPlugin::releaseResources() on all loaded plugins
	 *        this class is aware about.
	 *
	 * This works even if a certain plugin was loaded not by this class.
	 * If resources are not released explicitly, they will be released
	 * when unloading plugins, which happens after main() returns.
	 * Unfortunately, at least some OpenCL implementations don't like
	 * being accessed after main() returns.
	 */
	void releaseResources();

	/**
	 * @brief Delegates to a plugin, if one is loaded or returns a non-accelerated version.
	 */
	std::shared_ptr<AcceleratableOperations> getOperations();
private:
	AccelerationPlugin* m_pPlugin;
	std::shared_ptr<AcceleratableOperations> m_ptrNonAcceleratedOperations;
};

#endif
