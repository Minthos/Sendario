import numpy as np
import matplotlib.pyplot as plt

# Set the simulation parameters
dt = 0.01
t_final = 10
t = np.arange(0, t_final, dt)
n_steps = len(t)
mass = 1

# Initialize arrays to store the positions and velocities
x_euler = np.zeros(n_steps)
v_euler = np.zeros(n_steps)
x_verlet = np.zeros(n_steps)
v_verlet = np.zeros(n_steps)
x_your_method = np.zeros(n_steps)
v_your_method = np.zeros(n_steps)

# Initial conditions
x_euler[0] = 1
v_euler[0] = 0
x_verlet[0] = 1
v_verlet[0] = 0
x_your_method[0] = 1
v_your_method[0] = 0
prev_force = -x_your_method[0]
accumulated_force = -x_your_method[0]

# Time-stepping loop
for i in range(n_steps-1):
    # Euler method
    v_euler[i+1] = v_euler[i] - dt*x_euler[i]
    x_euler[i+1] = x_euler[i] + dt*v_euler[i]

    # Verlet method
    x_verlet[i+1] = x_verlet[i] + dt*v_verlet[i] - 0.5*dt**2*x_verlet[i]
    v_verlet[i+1] = v_verlet[i] - 0.5*dt*(x_verlet[i] + x_verlet[i+1])

    # Your method
    averageForce = (prev_force + accumulated_force) * 0.5
    dv = averageForce * (dt / mass)
    newVelocity = v_your_method[i] + dv
    x_your_method[i+1] = x_your_method[i] + dt * (newVelocity + v_your_method[i]) * 0.5
    v_your_method[i+1] = newVelocity
    prev_force = accumulated_force
    accumulated_force = -x_your_method[i+1]

# Analytical solution
x_exact = np.cos(t)

# Calculate the errors
error_euler = np.abs(x_euler - x_exact)
error_verlet = np.abs(x_verlet - x_exact)
error_your_method = np.abs(x_your_method - x_exact)

# Plot the errors
plt.figure()
plt.plot(t, error_euler, label='Euler')
plt.plot(t, error_verlet, label='Verlet')
plt.plot(t, error_your_method, label='Your method')
plt.legend()
plt.xlabel('Time')
plt.ylabel('Error')
plt.show()

