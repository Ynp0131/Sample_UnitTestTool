#pragma once

// Model層のツールが共通して持つインターフェース。
class Tool {
public:
    virtual ~Tool() = default;
    virtual void execute() = 0;
};
