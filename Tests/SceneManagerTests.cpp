#include "SceneManager.h"
#include <cassert>
#include <string>
#include <vector>

std::vector<std::string> events;
struct TestScene : Scene
{
    explicit TestScene(std::string name) : name(name) { events.push_back(name + ".create"); }
    ~TestScene() override { events.push_back(name + ".destroy"); }
    void Initialize() override { events.push_back(name + ".initialize"); }
    void Finalize() override { events.push_back(name + ".finalize"); }
    void Update(float) override { events.push_back(name + ".update"); }
    void Render(float) override { events.push_back(name + ".render"); }
    void DrawGUI() override { events.push_back(name + ".gui"); }
    std::string name;
};
int main()
{
    auto& manager = SceneManager::Instance();
    manager.Clear();
    manager.ChangeScene([] { return std::make_shared<TestScene>("a"); });
    assert(events.empty());
    manager.Update(0);
    manager.Render(0);
    manager.DrawGUI();
    assert((events == std::vector<std::string>{
        "a.create", "a.initialize", "a.update", "a.render", "a.gui"}));
    events.clear();
    manager.ChangeScene([] { return std::make_shared<TestScene>("b"); });
    assert(events.empty());
    manager.Update(0);
    assert((events == std::vector<std::string>{
        "a.finalize", "a.destroy", "b.create", "b.initialize", "b.update"}));
    events.clear();
    manager.ChangeScene([] { return std::make_shared<TestScene>("cancelled"); });
    manager.Clear();
    manager.Update(0);
    manager.Render(0);
    manager.DrawGUI();
    manager.Clear();
    assert((events == std::vector<std::string>{"b.finalize", "b.destroy"}));
}
