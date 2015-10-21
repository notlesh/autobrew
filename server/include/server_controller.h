#ifndef __AB2_SERVER_CONTROLLER_INCLUDED__
#define __AB2_SERVER_CONTROLLER_INCLUDED__

#include <atomic>

#include <roller/core/types.h>
#include <roller/core/thread.h>

using namespace roller;

/**
 * ServerController base class. A Servercontroller is a dedicated thread that can be started and stopped.
 *
 * Sub classes should override doRun() and should frequently check the isCancelled() flag
 * in order to stop quickly.
 */
class ServerController {

public:

	/**
	 * Constructor. The thread will not be started until start() is called.
	 */
	ServerController();

	/**
	 * Destructor. If the thread is currently running, this will join. If this is unacceptable,
	 * call stop() prior to destroying the controller.
	 */
	virtual ~ServerController();

	/**
	 * Do run -- main entry point for thread. Override this to perform controller logic.
	 */
	virtual void doRun() = 0;

	/**
	 * Check whether the controller has been cancelled. This should be called frequently
	 * inside the doRun() implementation in order to stop the thread from running quickly.
	 */
	bool isCancelled();

	/**
	 * Start the thread. Call this to kick off the thread. Should only be called once.
	 */
	void start();

	/**
	 * Stop the thread. Can be called whether the thread has finished or not, but should
	 * only be called after the thread has been started.
	 *
	 * If the underlying thread is still running at the time this is called, the controller
	 * will be considered cancelled. Otherwise the thread must have already completed and
	 * this will have no effect.
	 */
	void stop();

	/**
	 * Returns whether or not the controller has finished. A controller is considered finished
	 * if its thread has returned.
	 */
	bool isFinished();

	/**
	 * Joins the thread. This is safe to call regardless of the state of the controller.
	 */
	void join();

private:

	/**
	 * Our thread entry point.
	 */
	void run();

	Thread _thread;
	std::atomic_bool _cancelled;
	std::atomic_bool _started;
	std::atomic_bool _finished;
};

#endif // __AB2_SERVER_CONTROLLER_INCLUDED__
