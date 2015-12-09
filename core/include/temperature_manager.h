#ifndef __AB_TEMPERATURE_MANAGER_H
#define __AB_TEMPERATURE_MANAGER_H

#include <map>
#include <list>
#include <utility>
#include <memory>
#include <set>
#include <functional>

#include "hw_manager.h"

#include <roller/core/string_id.h>
#include <roller/core/thread.h>
#include <roller/core/mutex.h>
#include <roller/core/indexed_container.h>

#define NUM_TEMP_SAMPLES 131072
#define SAMPLE_FILL_FREQUENCY_MS 1000

using namespace roller;
using namespace devman;
using std::map;
using std::list;
using std::pair;
using std::unique_ptr;
using std::function;
using std::set;

#define ABS_ZERO_CELCIUS -273.15

/**
 * ProbeSettings struct
 */
struct ProbeSettings {

	StringId _id;
	StringId _managerId;
	string _name;
	// BrewRole _role;
	// Color _color;
	bool _showThermometer;
	bool _showInGraphs;
};

/**
 * ProbeStats struct
 */
struct ProbeStats {
	StringId _id;
	i32 _lastTemp;
	i64 _firstSeen;
	i64 _lastSeen;
	i64 _numSuccess;
	i64 _numErrors;
};

typedef function<void(const ProbeStats& before, const ProbeStats& after)> ProbeStatsListener;
typedef function<void(const ProbeSettings& settings, const ProbeStats& stats)> NewProbeListener;

/**
 * TemperatureManager manages temperatures. It will poll temperatures in a separate thread. The
 * latest temperature can always be obtained. Obtaining a temperature from the manager requires a
 * mutex lock and unlock.
 *
 * TemperatureManager is a thread. To start it, simply call start (Thread::start() ).
 *
 * TemperatureManager uses devman to obtain temperature readings. The user should configure 
 * devman with the proper device managers before starting TemperatureManager.
 */
class TemperatureManager : public Thread {

public:

	/**
	 * Constructor
	 */
	TemperatureManager();

	/**
	 * Destructor. Implicitly calls join()
	 */
	~TemperatureManager();

	/**
	 * Stop the thread
	 */
	void stop();

	/**
	 * Sets UpdateFrequency
	 *
	 * @param updateFrequency is the new value for UpdateFrequency
	 */
	void setUpdateFrequency( i32 updateFrequency );
	
	/**
	 * Returns UpdateFrequency
	 *
	 * @return UpdateFrequency
	 */
	i32 getUpdateFrequency() const;

	/**
	 * List known probes
	 */
	list<StringId> listProbes();

	/**
	 * Get the probe stats for the given probe
	 *
	 * @param sensorId is the id of the sensor
	 * @return the ProbeStats for the given probe
	 */
	const ProbeStats& getProbeStats( const StringId& sensorId ) const;

	/**
	 * Returns a raw pointer to a temperature sample buffer. This should be treated as read only. If
	 * no buffer is available for the given sensor, nullptr will be returned.
	 */
	f64* getBuffer( const StringId& sensorId );

	/**
	 * Returns the sample rate (the rate at which the stored buffers are filled)
	 */
	i32 getSampleRate();

	/** 
	 * Returns the time of the first sample taken.
	 */
	i64 getFirstBufferSampleTime();

	/** 
	 * Returns the time of the most recent sample taken.
	 */
	i64 getMostRecentBufferSampleTime();

	/**
	 * Add a ProbeStats listener. This is threadsafe.
	 */
	Key addStatsListener( const ProbeStatsListener& listener );

	/**
	 * Remove a ProbeStats listener. This is threadsafe.
	 */
	void removeStatsListener( const Key& key );

	/**
	 * Add a new probe listener. This is threadsafe.
	 */
	Key addNewProbeListener( const NewProbeListener& listener );

	/**
	 * Remove a new probe listener. This is threadsafe.
	 */
	void removeNewProbeListener( const Key& key );

private:

	/**
	 * Run the thread.
	 */
	void doRun();

	/**
	 * Update probe list.
	 */
	void updateProbeList();

	/**
	 * Update temperatures.
	 */
	void updateTemperatures();

	/**
	 * Dump temp data. This is a crude hack to make temp data available outside the
	 * process (http, etc.)
	 *
	 * It is hard coded to dump data in HTML format to:
	 *      /var/www/temp_data.html
	 * And as JSON to:
	 *      /var/www/json_data.html
	 *
	 * Currently called at the end of the main update loop (after updateTemperatures())
	 */
	void dumpTempData();

	/**
	 * Fill sample values
	 */
	void fillSamples( i32 index );

	/**
	 * Helper to fire probe stats changed events
	 */
	void fireProbeStatsChangedEvent( const ProbeStats& before, const ProbeStats& after );

	/**
	 * Helper to fire probe added events
	 */
	void fireProbeAddedEvent( const ProbeSettings& settings, const ProbeStats& stats );

	mutable Mutex _dataLock;
	bool _running;
	set<pair<StringId, StringId>> _knownProbes;
	map<StringId, ProbeStats> _probeStats;

	map<StringId, i32> _readings;
	map<StringId, i64> _errorCounts;
	map<StringId, i64> _successCounts;
	map<StringId, unique_ptr<f64[]>> _buffers;

	i32 _updateFrequency;
	i64 _lastUpdate;

	i32 _updateProbeListFrequency;
	i64 _lastUpdateProbeList;

	i64 _firstSampleTime;
	i64 _lastSampleFill;

	Mutex _eventLock;
	IndexedContainer<ProbeStatsListener> _probeStatsListeners;
	IndexedContainer<NewProbeListener> _newProbeListeners;
	
};

#endif // __AB_TEMPERATURE_MANAGER_H
