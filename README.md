![image](https://github.com/user-attachments/assets/20c7ccc5-265f-41fc-8901-47f86442d631)
![image](https://github.com/user-attachments/assets/2cb85c87-38ba-4f78-8ca2-f0c0d5937044)
About
---------------------------------------------------------------------------------------------

My thesis is a deep dive into 3D AI navigation, specifically blending multithreaded A* pathfinding with dynamic obstacle avoidance using ORCA. The world is procedurally generated using perlin noise, and from that I use the heightmap data to construct a NavMesh. The NavMesh is composed of triangles which allow the agents to traverse through the environment.
Each AI agent computes paths asynchronously using a multithreaded A* system I designed, which significantly improves scalability. On top of pathfinding, I integrated ORCA (Optimal Reciprocal Collision Avoidance) so agents can dynamically adjust their velocities in real time to avoid collisions with others in dense scenarios. This project also consist of multiple game modes showcasing the different velocity-based avoidance algorithms comparing how each algorithm works within similar scenarios. 

How To Use
---------------------------------------------------------------------------------------------
The game also provides you with instructions on how to play the game and what you need to do to beat the game. 

The Keyboard Controls:
1.	W and S key to move in the forward and backward direction
2.	A and D key to move left or right
3.	Hold shift to sprint
4.	Press P to toggle pausing and unpausing
5.	Press F11 to toggle showing the cursor 
6.	Use the mouse to look in any direction which dictates the direction you will move forward in 

The rest of the debug keys and information regarding what they do is in the UI for each game mode.

How To Run Application
---------------------------------------------------------------------------------------------
Before anything you must have the Navisyn-Engine already downloaded as this won't run without it. It must also be in the same file path as this project.

Method A:
1.	Extract the zip folder to your desired location
2.	Open the following path --> …\DFS2MarkovChains\Run
3.	Double-click DFS2MarkovChains_Release_x64.exe to start the program
   
Method B:
1.	Extract the zip folder to you desired location.
2.	Open the following path --> …\DFS2MarkovChains
3.	Open the DFS2MarkovChains.sln using Visual Studio 2022 and make sure the solution config and platforms are "Release" and "x64".
4.	Press F6 key to build solution or go to Build --> Build Solution option using the Menu bar.
5.	Press Ctrl + F5 key to start the program without debugging or go to Debug --> Start without Debugging option using the Menu bar.
  NOTE:	
	  * Visual Studio 2022 must be installed on the system.
	  * In step 5 mentioned above, if you want you can execute the program with the debugger too.
