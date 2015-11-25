#include "temperature_manager.h"

#include <unistd.h>
#include <fstream>
#include <iostream>

#include "device_manager.h"

#include <roller/core/util.h>

using namespace devman;
using std::ofstream;
using std::endl;
using std::ios;


// Constructor
TemperatureManager::TemperatureManager() :
				Thread( std::bind( &TemperatureManager::doRun, this )),
				_dataLock(true),
				_running(false),
				_updateFrequency(333),
				_lastUpdate(0),
				_updateProbeListFrequency(15000),
				_lastUpdateProbeList(0),
				_firstSampleTime(-1),
				_lastSampleFill(-1),
				_eventLock(true) {
	_lastUpdate = getTime() + 3000;
}

// stop
void TemperatureManager::stop() {
	_running = false;
}

// setUpdateFrequency
void TemperatureManager::setUpdateFrequency( i32 updateFrequency ) {
	_updateFrequency = updateFrequency;
};

//  getUpdateFrequency
i32 TemperatureManager::getUpdateFrequency() const {
	return _updateFrequency;
};

// listProbes
list<StringId> TemperatureManager::listProbes() {
	list<StringId> probes;

	// Log::i( "TemperatureManager::listProbes()" );

	_dataLock.lock();
	for ( auto entry : _knownProbes ) {
		probes.push_back( entry.second );
	}
	_dataLock.unlock();

	return probes;
}

// getProbeStats
const ProbeStats& TemperatureManager::getProbeStats( const StringId& sensorId ) const {

	_dataLock.lock();
	auto itr = _probeStats.find( sensorId );
	if ( itr != _probeStats.end() ) {
		_dataLock.unlock();
		return itr->second;
	} else {
		_dataLock.unlock();
		throw RollerException( "No such probe stats: %s", sensorId.getString().c_str() );
	}
}

// getBuffer
f64* TemperatureManager::getBuffer( const StringId& sensorId ) {

	f64* bufferAddr = nullptr;
	bool present = false;

	_dataLock.lock();
	auto itr = _buffers.find( sensorId );
	if ( itr != _buffers.end() ) {
		bufferAddr = itr->second.get();
		present = true;
	}
	_dataLock.unlock();

	if ( ! present ) {
		throw RollerException( "No such temperature: %s", sensorId.getString().c_str() );
	} else {
		return bufferAddr;
	}
}

// getSampleRate
i32 TemperatureManager::getSampleRate() {
	return SAMPLE_FILL_FREQUENCY_MS;
}

// getFirstBufferSampleTime
i64 TemperatureManager::getFirstBufferSampleTime() {
	return _firstSampleTime;
}

// getMostRecentBufferSampleTime
i64 TemperatureManager::getMostRecentBufferSampleTime() {
	return _lastSampleFill;
}

// addStatsListener
Key TemperatureManager::addStatsListener( const ProbeStatsListener& listener ) {
	_eventLock.lock();
	Key key = _probeStatsListeners.add( listener );
	_eventLock.unlock();
	return key;
}

// removeStatsListener
void TemperatureManager::removeStatsListener( const Key& key ) {
	_eventLock.lock();
	_probeStatsListeners.remove( key );
	_eventLock.unlock();
}

// addNewProbeListener
Key TemperatureManager::addNewProbeListener( const NewProbeListener& listener ) {
	_eventLock.lock();
	Key key = _newProbeListeners.add( listener );
	_eventLock.unlock();
	return key;
}

// removeNewProbeListener
void TemperatureManager::removeNewProbeListener( const Key& key ) {
	_eventLock.lock();
	_newProbeListeners.remove( key );
	_eventLock.unlock();
}

// doRun
void TemperatureManager::doRun() {

	_running = true;

	// XXX: hack
	usleep( 10000000 );

	while ( _running ) {

		i64 now = getTime();
		i64 elapsed = now - _lastUpdate;

		if ( elapsed < _updateFrequency ) {
			// sleep for 3 ms (don't want to sleep too long, otherwise we might
			// block when we want to exit the app)
			usleep( 3000 );
			continue;
		}

		// update probe list if appropriate
		if ( now - _lastUpdateProbeList > _updateProbeListFrequency ) {
			try {
				updateProbeList();
			} catch ( const exception& e ) {
				Log::w( "Exception while trying to get probe list, will try again: %s", e.what() );
				// TODO: detect multiple failures in a row
				continue;
			} catch ( ... ) {
				Log::w( "Unrecognized exception while trying to get probe list, will try again" );
				// TODO: detect multiple failures in a row
				continue;
			}
			_lastUpdateProbeList = now;
		}

		// update temperatures
		updateTemperatures();
		_lastUpdate = now;

		dumpTempData();

		while ( (_firstSampleTime > 0) && now - _lastSampleFill > SAMPLE_FILL_FREQUENCY_MS ) {
			// Log::i( "now: %ld, last: %ld, first: %ld, sample rate: %d", now, _lastSampleFill, _firstSampleTime, SAMPLE_FILL_FREQUENCY_MS );
			fillSamples( (_lastSampleFill - _firstSampleTime) / SAMPLE_FILL_FREQUENCY_MS );
			_lastSampleFill += SAMPLE_FILL_FREQUENCY_MS;
		}

	}
}

// updateProbeList
void TemperatureManager::updateProbeList() {

	// Log::i( "TemperatureManager::updateProbeList()" );

	auto probes = DeviceManager::listTemperatureSensors();
	i64 now = getTime();

	set<pair<StringId, StringId>> newProbes;

	for ( auto entry : probes ) {
		if ( _knownProbes.find( entry ) == _knownProbes.end() ) {
			newProbes.insert( entry );
		}
	}

	// lock and copy new probes to list and create buffers
	_dataLock.lock();
	for ( auto entry : newProbes ) {
		_knownProbes.insert( entry );

		StringId sensorId = entry.second;

		auto itr = _buffers.find( sensorId );
		if ( itr == _buffers.end() ) {
			unique_ptr<f64[]> buffer( new f64[NUM_TEMP_SAMPLES] );
			buffer[0] = ABS_ZERO_CELCIUS;
			memfill( buffer.get(), NUM_TEMP_SAMPLES * sizeof(f64), sizeof(f64) );
			_buffers[sensorId] = std::move( buffer );
		}

		// TODO: check stats (from database, etc.)
		ProbeStats& stats = _probeStats[sensorId] = {
				sensorId,
				-1,
				now,
				now, 
				0,
				0 };

		// const ProbeSettings& settings = g_prefManager.primeProbeSettings( sensorId, entry.first );

		// fireProbeAddedEvent( settings, stats );

	}
	_dataLock.unlock();

	// TODO: detect probes removed?
}

// updateTemperatures
void TemperatureManager::updateTemperatures() {
	// Log::i( "TemperatureManager::updateTemperatures()" );

	if ( _firstSampleTime < 0 ) {
		_firstSampleTime = getTime();
	}

	for ( auto entry : _knownProbes ) {

		// TODO: handle error

		StringId sensorId = entry.second;
		auto sensor = DeviceManager::getTemperatureSensor( entry.first, entry.second );

		i64 time = 0;
		i32 temp = 0;

		_dataLock.lock();
		auto itr = _probeStats.find( sensorId );
		if ( itr == _probeStats.end() ) {
			_dataLock.unlock();
			Log::w( "Invalid probe in probes list (ignoring): %s", sensorId.getString().c_str() );
			continue;
		}
		ProbeStats& stats = itr->second;
		ProbeStats oldStats = stats;
		_dataLock.unlock();

		try {
			temp = sensor->getTemperature( time );

			// Log::i( "%lld: %s = %d", time, sensorId.getString().c_str(), temp );
			
			_dataLock.lock();
			stats._numSuccess++;
			stats._lastTemp = temp;
			stats._lastSeen = time;
			_dataLock.unlock();

		} catch ( const exception& e ) {

			_dataLock.lock();
			stats._numErrors++;

			Log::w( 
					"Exception while trying to read probe %s (error count: %ld): \n  %s",
					sensorId.getString().c_str(),
					stats._numErrors,
					e.what() );
			_dataLock.unlock();
		} catch ( ... ) {

			_dataLock.lock();
			stats._numErrors++;

			Log::w( 
					"Exception while trying to read probe %s (error count: %ld)",
					sensorId.getString().c_str(),
					stats._numErrors );
			_dataLock.unlock();
		}

		fireProbeStatsChangedEvent( oldStats, stats );


		// Log::i( " - %s: %d", sensorId.getString().c_str(), temp );

		// TODO: review this
		_dataLock.lock();
		_readings[sensorId] = temp;
		_dataLock.unlock();

	}

	if ( _lastSampleFill < 0 ) {
		fillSamples( 0 );
		_lastSampleFill = _firstSampleTime;
	}
}

// dumpTempData
void TemperatureManager::dumpTempData() {

	// dump as html
	auto probeIds = listProbes();

	ofstream htmlOut;
	htmlOut.open( "/var/www/temp_data.html", ios::trunc | ios::out );

	// html bullshit
	htmlOut << "<html><head><meta http-equiv=\"refresh\" content=\"3\" /></head><body>" << endl;
	htmlOut << "<table border=\"1\" cellpadding=\"3\" cellspacing=\"3\">" << endl;

	for ( auto probeId : probeIds ) {

		htmlOut << "<tr>";

		auto probeStats = getProbeStats( probeId );

		i32 temp = probeStats._lastTemp;
		f32 c = (f32)temp / 1000.0f;
		f32 f = (9.0f / 5.0f) * c + 32.0f;

		htmlOut		<< "<td>"
				    << probeId.getString()
				    << "</td><td>"
				    << c << "c"
				    << "</td><td>"
				    << f << "f"
				    << "</td></tr>" << endl;

	}

	htmlOut << "</table></body></html>";

	htmlOut.flush();

	// now do the same, but in json format
	ofstream jsonOut;
	jsonOut.open( "/var/www/temp_data.json", ios::trunc | ios::out );

	jsonOut << "{\n";

	size_t counter = 0;
	for ( auto probeId : probeIds ) {

		counter++;

		jsonOut << "    \"" << probeId.getString() << "\": {\n";

		auto probeStats = getProbeStats( probeId );

		i32 temp = probeStats._lastTemp;
		f32 c = (f32)temp / 1000.0f;
		f32 f = (9.0f / 5.0f) * c + 32.0f;

		jsonOut << "        \"tempC\": " << c << ",\n"
			    << "        \"tempF\": " << f << ",\n"
			    << "        \"lastSeen\": " << probeStats._lastSeen << "\n";

		if (counter == probeIds.size()) {
			jsonOut << "    }\n";
		} else {
			jsonOut << "    },\n";
		}
	}

	jsonOut << "}\n";
	
	jsonOut.flush();
}

// fillSamples
void TemperatureManager::fillSamples( i32 index ) {

	if ( index >= NUM_TEMP_SAMPLES ) {
		Log::w( "FIXME: We filled up temp sample buffer! Been brewing for a while?" );
		Log::w( "       Index: %d, Buffer size: %d", index, NUM_TEMP_SAMPLES );
		Log::w( "       Expect bad things to happen." );
	}

	for ( auto entry : _knownProbes ) {

		StringId sensorId = entry.second;
		
		auto buffersItr = _buffers.find( sensorId );
		auto readingsItr = _readings.find( sensorId );

		if ( buffersItr == _buffers.end() ) {
			Log::w( "FIXME: Can't fill probe temp buffer, buffer doesn't exist!" );
			continue;
		}

		if ( readingsItr == _readings.end() ) {
			// means no reading is available for this probe yet
			continue;
		}

		_buffers[sensorId][index] = _readings[sensorId];

	}
}

// fireProbeStatsChangedEvent
void TemperatureManager::fireProbeStatsChangedEvent( const ProbeStats& before, const ProbeStats& after ) {

	_eventLock.lock();
	for ( auto callback : _probeStatsListeners ) {
		try {
			callback( before, after );
		} catch ( const exception& e ) {
			Log::w( "Warning: exception caught while trying to make callback (ignoring): %s", e.what() );
		} catch ( ... ) {
			Log::w( "Warning: unrecognized exception caught while trying to make callback (ignoring)" );
		}
	}
	_eventLock.unlock();
}

// fireProbeAddedEvent
void TemperatureManager::fireProbeAddedEvent( const ProbeSettings& settings, const ProbeStats& stats ) {

	Log::i( "Firing probe added event (%s)", settings._id.getString().c_str() );

	_eventLock.lock();
	for ( auto callback : _newProbeListeners ) {
		try {
			callback( settings, stats );
		} catch ( const exception& e ) {
			Log::w( "Warning: exception caught while trying to make callback (ignoring): %s", e.what() );
		} catch ( ... ) {
			Log::w( "Warning: unrecognized exception caught while trying to make callback (ignoring)" );
		}
	}
	_eventLock.unlock();
}
