#pragma once

class ILauncher {
public:
    virtual ~ILauncher() = default;
    virtual bool prepare() = 0;
    virtual void start() = 0;
};
