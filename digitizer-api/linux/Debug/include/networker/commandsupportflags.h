#pragma once

namespace network
{
namespace api_check
{

bool inline isUpdateRootFSCommandAvailable(int major, int minor, int patch)
{
    return major >= 1 && minor >= 4;
}

bool inline isStartWithTimeCommandAvailable(int major, int minor, int patch)
{
    return major >= 1 && minor >= 4;
}

} // namespace api_check

} // namespace network