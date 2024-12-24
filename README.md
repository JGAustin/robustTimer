# robustTimer
This is a basic and robust timer class for C++ applications requiring precise timing control. It utilized posix timers to post a semaphore in the signal handler. It contains a second thread which waits for the semaphore to call the callback function.

I am a realtime software engineer of 8 years and need this functionality all the time. To expidite the process of writing this, I gave a specific architecture to ChatGPT and using it to streamline the implementation process.
