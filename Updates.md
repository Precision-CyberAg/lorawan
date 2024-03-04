
All simulation test scenarios for performance evaluation of LoRaWAN Class B and Class C devices are implemented using <br>
<a href='https://github.com/Precision-CyberAg/lorawan/blob/develop/examples/lora-device-classes-example.cc'>lora-device-classes-example.cc</a>

This example file is executed from a different runner project written with python, <br><a href='https://github.com/phanijsp/lora-sim-run-book'>lora-sim-run-book</a>

It is important to have both the projects in the same environment since the runner project requires a fully compiled ns-3 with our updated <a href='https://github.com/Precision-CyberAg/lorawan/'>LoRaWAN</a> module.

In the runner project, the notebook files needs updated "command" variable that sets the right ns-3 installed path. 

Author
- Aditya Jagatha<br>
  aditya.jagatha@trojans.dsu.edu / phani.jsp@gmail.com
