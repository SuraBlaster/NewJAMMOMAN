#pragma once
#include "ICommand.h"
#include <stack>
#include <memory>

class CommandManager
{
public:
    void Execute(std::unique_ptr<ICommand> command);
    void Undo();
    void Redo();
    void Clear();

    bool CanUndo() const { return !undoStack.empty(); }
    bool CanRedo() const { return !redoStack.empty(); }

private:
    std::stack<std::unique_ptr<ICommand>> undoStack;
    std::stack<std::unique_ptr<ICommand>> redoStack;
};
