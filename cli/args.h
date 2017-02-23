#ifndef CLI_ARGS_H
#define CLI_ARGS_H

#include <c++utilities/application/argumentparser.h>

namespace Cli {

using namespace ApplicationUtilities;

struct Args
{
    Args();
    ArgumentParser parser;
    HelpArgument help;
    OperationArgument status, log, stop, restart, rescan, rescanAll, pause, pauseAllDevs, pauseAllDirs, resume, resumeAllDevs, resumeAllDirs, waitForIdle;
    ConfigValueArgument statusDir, statusDev, pauseDir, pauseDev;
    ConfigValueArgument configFile, apiKey, url, credentials, certificate;
};

} // namespace Cli

#endif // CLI_ARGS_H
