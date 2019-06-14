//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//
import inet.visualizer.integrated.IntegratedVisualizer;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.inet.StandardHost;
import inet.node.inet.WirelessHost;
import inet.node.inet.AdhocHost;
import inet.physicallayer.unitdisk.UnitDiskRadioMedium;
import inet.networklayer.configurator.ipv4.HostAutoConfigurator;

network Drones
{
    parameters:
        int numHosts;

    submodules:
        visualizer: IntegratedVisualizer {
            @display("p=100,100");
        }
        radioMedium: UnitDiskRadioMedium {
            @display("p=100,500");
        }

        host[numHosts]: Drone;
        clone: Clone {
            @dynamic;
            @display("r=250,red");
            @display("i=misc/node_vs");
        }
}
