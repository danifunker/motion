#include <coherent/coherent.hpp>
#include <component/ip2/ip2_duart.hpp>

namespace Iris
{  
    void CoherentExtensionDUART68681::AddUI()
    {
        if (ImGui::Begin("DUART State"))
        {
            ImGui::TextColored(ImVec4(1.0, 0.0, 0.0, 1.0), "TODO: Everything!!");
            ImGui::End();
        }
    }   
}