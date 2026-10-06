#include "CommandManager.h"

void CommandManager::Execute(std::unique_ptr<ICommand> command)
{
    command->Execute();
    undoStack.push(std::move(command));
    while (!redoStack.empty())
        redoStack.pop();
}

void CommandManager::Undo()
{
    if (undoStack.empty()) return;

    auto cmd = std::move(undoStack.top());
    undoStack.pop();
    cmd->Undo();
    redoStack.push(std::move(cmd));
}

void CommandManager::Redo()
{
    if (redoStack.empty()) return;

    auto cmd = std::move(redoStack.top());
    redoStack.pop();
    cmd->Execute();
    undoStack.push(std::move(cmd));
}

void CommandManager::Clear()
{
    while (!undoStack.empty()) undoStack.pop();
    while (!redoStack.empty()) redoStack.pop();
}