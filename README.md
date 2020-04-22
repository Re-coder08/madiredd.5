
Users send their resource request to OSS through a message queue. OSS collects a bunch of messages,
and dispatches them.

For our deadlock detection we use the Bankers algorithm, to detech if current requests are in a deadlock situation.
If such a situation is found, we select the user with highest number of resources, and terminate it.

Detection is executed, if we can't dispatch a process.
