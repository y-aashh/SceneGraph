#include <iostream>
#include <string>
#include <unordered_map>
#include <cmath>
#include<forward_list>
#include <queue>
#include<thread>
#include<mutex>


class IObserver {
    public:
    virtual ~IObserver() {}
    virtual void update() = 0;
};

class Subject {
public:
    void addObserver(IObserver* observer) {
        observers.push_front(observer);
    }

    virtual void removeObserver(IObserver* observer){
        observers.remove(observer);
    }

    void notify() {
        for (IObserver* observer : observers) {
            observer->update();
        }
    }

private:
    std::forward_list<IObserver*> observers;
};

class SceneNode {
private:
    double transformMatrix[3][3];

public:
    SceneNode() {
        // Initialize identity matrix
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                transformMatrix[i][j] = (i == j) ? 1.0 : 0.0;
            }
        }
    }
    void displaySceneNode() const {
        std::cout << "Transformation Matrix:" << std::endl;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                std::cout << transformMatrix[i][j] << " ";
            }
            std::cout << std::endl;
        }
    }

    void rotate(double angle) {
        double rotationMatrix[3][3] = {
            {cos(angle), -sin(angle), 0},
            {sin(angle), cos(angle), 0},
            {0, 0, 1}
        };

        multiplyMatrix(transformMatrix, rotationMatrix);
    }

    void scale(double scaleX, double scaleY) {
        double scalingMatrix[3][3] = {
            {scaleX, 0, 0},
            {0, scaleY, 0},
            {0, 0, 1}
        };

        multiplyMatrix(transformMatrix, scalingMatrix);
    }

    void translate(double tx, double ty) {
        double translationMatrix[3][3] = {
            {1, 0, tx},
            {0, 1, ty},
            {0, 0, 1}
        };

        multiplyMatrix(transformMatrix, translationMatrix);
    }

private:

    void multiplyMatrix(double A[3][3], double B[3][3]) {
        double result[3][3] = {{0}};

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                for (int k = 0; k < 3; ++k) {
                    result[i][j] += A[i][k] * B[k][j];
                }
            }
        }

        // Copy result to transformMatrix
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                transformMatrix[i][j] = result[i][j];
            }
        }
    }
};

class Cube : public SceneNode {
public:
    void sphereCollider() {}
};

// Derived class representing a Sphere
class Sphere : public SceneNode {
public:
    void boxCollider() {}
};

class MetaData {
public:
    std::shared_ptr<SceneNode> treeNode;
    std::string parentName;

    MetaData() {}
    MetaData(std::shared_ptr<SceneNode> node, std::string parent) : treeNode(node), parentName(parent) {}
};

class Mapper : public Subject {
private:
    std::unordered_map<std::string, MetaData> dataMap;

    // Private constructor to prevent instantiation from outside
    Mapper() {
        // Initialize with a root node
        std::shared_ptr<SceneNode> rootNode = nullptr;
        MetaData rootMetaData(rootNode, "");
        dataMap["root"] = rootMetaData;
    }

public:
    // Static function to get the singleton instance
    static Mapper& getInstance() {
        static Mapper instance;
        return instance;
    }

    // Delete copy constructor and assignment operator to prevent duplication
    Mapper(const Mapper&) = delete;
    void operator=(const Mapper&) = delete;

    void addNode(const std::string& name, std::shared_ptr<SceneNode> node, const std::string& parentName) {

        std::lock_guard<std::mutex> lock(mutex);
        MetaData metaData(node, parentName);
        dataMap[name] = metaData;
        notify();

        
    }

    MetaData* search(const std::string& name) {
            
        std::lock_guard<std::mutex> lock(mutex);
        auto it = dataMap.find(name);
        if (it != dataMap.end()) {
            return &(it->second);
        }
        return nullptr;
        
    }

    void updateParentName(const std::string& name, const std::string& newParentName) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = dataMap.find(name);
        auto pt = dataMap.find(newParentName);

        if (it != dataMap.end() && pt != dataMap.end()) {
            it->second.parentName = newParentName;
        } else {
            std::cout << "Reparenting Failed " << std::endl;
        }
        notify();
    }

    void displayTree() {
        std::cout << "Tree Structure:" << std::endl;
        displaySubtree("root", 0);
    }

    void deleteNodeAndChildren(const std::string& nodeName) {
        std::lock_guard<std::mutex> lock(mutex);
        auto nodeToDelete = dataMap.find(nodeName);
        if (nodeToDelete != dataMap.end()) {
            std::string parentName = nodeToDelete->second.parentName;

            // Delete children nodes
            for (auto it = dataMap.begin(); it != dataMap.end();) {
                if (it->second.parentName == nodeName) {
                    it = dataMap.erase(it);
                } else {
                    ++it;
                }
            }

            // Delete the node itself
            dataMap.erase(nodeName);
        }
        notify();
    }

    void rotateNodeAndChildren(const std::string& nodeName, double angle) {
        std::lock_guard<std::mutex> lock(mutex);
        std::queue<std::string> nodeQueue;
        nodeQueue.push(nodeName);

        while (!nodeQueue.empty()) {
            std::string currentNodeName = nodeQueue.front();
            nodeQueue.pop();

            auto node = dataMap.find(currentNodeName);
            if (node != dataMap.end()) {
                std::shared_ptr<SceneNode> treeNode = node->second.treeNode;
                treeNode->rotate(angle); // Rotate the current node

                // Enqueue children nodes for processing
                for (const auto& data : dataMap) {
                    if (data.second.parentName == currentNodeName) {
                        nodeQueue.push(data.first);
                    }
                }
            }
        }

        notify();
    }

    void scaleNodeAndChildren(const std::string& nodeName, double sx, double sy) {
        std::lock_guard<std::mutex> lock(mutex);
        std::queue<std::string> nodeQueue;
        nodeQueue.push(nodeName);

        while (!nodeQueue.empty()) {
            std::string currentNodeName = nodeQueue.front();
            nodeQueue.pop();

            auto node = dataMap.find(currentNodeName);
            if (node != dataMap.end()) {
                std::shared_ptr<SceneNode> treeNode = node->second.treeNode;
                treeNode->scale(sx, sy); // Scale the current node

                // Enqueue children nodes for processing
                for (const auto& data : dataMap) {
                    if (data.second.parentName == currentNodeName) {
                        nodeQueue.push(data.first);
                    }
                }
            }
        }

        notify();
    }


    void translateNodeAndChildren(const std::string& nodeName, double tx, double ty) {
    std::lock_guard<std::mutex> lock(mutex);
    std::queue<std::string> nodeQueue;
    nodeQueue.push(nodeName);

        while (!nodeQueue.empty()) {
            std::string currentNodeName = nodeQueue.front();
            nodeQueue.pop();

            auto node = dataMap.find(currentNodeName);
            if (node != dataMap.end()) {
                std::shared_ptr<SceneNode> treeNode = node->second.treeNode;
                treeNode->translate(tx, ty); // Translate the current node

                // Enqueue children nodes for processing
                for (const auto& data : dataMap) {
                    if (data.second.parentName == currentNodeName) {
                        nodeQueue.push(data.first);
                    }
                }
            }
        }

        notify();
    }

    void displayNode(const std::string& nodeName) {

        MetaData* myNode = search(nodeName);
        if(myNode) {
            myNode->treeNode->displaySceneNode();
        }
        else {
            std::cout << "Could not find node " << nodeName << std::endl;
        }
    }

private:
    std::mutex mutex;
    void displaySubtree(const std::string& nodeName, int level) {
        for (int i = 0; i < level; ++i) {
            std::cout << "  ";
        }
        std::cout << "- " << nodeName << std::endl;

        // Find children of the current node
        for (const auto& pair : dataMap) {
            if (pair.second.parentName == nodeName) {
                displaySubtree(pair.first, level + 1);
            }
        }
    }
};

class Observer : public IObserver {
public:
    void update() {
        std::cout << "Client notified updated\n";
        Mapper& mapper = Mapper::getInstance();
        mapper.displayTree();
    }

    Observer() {
        Mapper& mapper = Mapper::getInstance();
        mapper.addObserver(this);
    }

};

class NodeFactory {
public:
    // Function to create a scene node based on the given type
    static std::shared_ptr<SceneNode> createNode(const std::string& type) {
        if (type == "Cube") {
            return std::make_shared<Cube>();
        } else if (type == "Sphere") {
            return std::make_shared<Sphere>();
        } else {
            // Invalid type
            return nullptr;
        }
    }

    // Function to add a scene node to the mapper
    static void addNodeToMapper(const std::string& name, const std::string& type, const std::string& parentName) {
        Mapper& mapper = Mapper::getInstance();
        std::shared_ptr<SceneNode> node = createNode(type);
        if (node) {
            mapper.addNode(name, node, parentName);
        } else {
            std::cout << "Error: Invalid node type." << std::endl;
        }
    }
};

class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty())
                            return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& worker : workers) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};


int main() {
    Mapper& mapper = Mapper::getInstance();

    Observer watcher;
    ThreadPool pool(4);

    pool.enqueue([&mapper] { NodeFactory::addNodeToMapper("cube1", "Cube", "root"); }); 
    pool.enqueue([&mapper] { NodeFactory::addNodeToMapper("sphere2", "Sphere", "sphere1"); }); 
    pool.enqueue([&mapper] { NodeFactory::addNodeToMapper("sphere3", "Sphere", "cube1"); }); 
    pool.enqueue([&mapper] { NodeFactory::addNodeToMapper("sphere4", "Sphere", "sphere3"); }); 
    
    pool.enqueue( [&mapper] { mapper.updateParentName("sphere3", "root"); });
    
    mapper.displayNode("cube1");
    mapper.displayNode("sphere3");
    mapper.displayNode("sphere4");
    
    pool.enqueue([&mapper] {  mapper.scaleNodeAndChildren("cube1", 2, 2); }); 
    pool.enqueue([&mapper] {  mapper.translateNodeAndChildren("cube1", 2, 2); }); 

    

    mapper.displayNode("cube1");
    mapper.displayNode("sphere3");
    mapper.displayNode("sphere4");
   
   pool.enqueue([&mapper] {  mapper.deleteNodeAndChildren("cube1"); }); 

    return 0;
}
