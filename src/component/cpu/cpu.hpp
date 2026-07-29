#pragma once 

#include <Iris.hpp>
#include <component/component.hpp>

namespace Iris
{
    /// @brief Base class for components that implement a CPU
    class ComponentCPU : public Component
    {
    public:

        /// @brief See Iris::Component::IsCPU
        /// @return See Iris::Component::IsCPU
        bool IsCPU() override { return true; };

        /// @brief get the name of this component. immutable const char*.
        const char* GetName() { return "CPU Generic Base Class (error)"; };

        /// @brief returns a boolean indicating if this cpu is in privileged mode. most cpus only have two levels of privilege and x86 has 4, but 2 are almost never used.
        /// @return a boolean indicating if the cpu is in privileged mode
        virtual bool IsPrivilegedMode() { return true; };
    };
}