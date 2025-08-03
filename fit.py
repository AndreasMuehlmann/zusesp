import numpy as np
import matplotlib.pyplot as plt

actualVelocity = np.array(
    [-2, 0, 12, 16, 20, 26, 29, 42, 45, 50, 58, 63, 66, 74, 80, 88, 96, 102])
pwm = np.array([0, 3, 9, 14, 19, 30, 45, 60, 68, 76,
               91, 98, 108, 123, 137, 152, 167, 182])

coeffs = np.polyfit(actualVelocity, pwm, 2)
a, b, c = coeffs
print(f"PWM = {a:.5f} * v^2 + {b:.5f} * v + {c:.5f}")

v_fit = np.linspace(min(actualVelocity), max(actualVelocity), 500)
pwm_fit = a * v_fit**2 + b * v_fit + c

plt.figure(figsize=(10, 6))
plt.plot(actualVelocity, pwm, 'ro', label="Messwerte (Dial shows v, PWM used)")
plt.plot(v_fit, pwm_fit, 'b-', label="Fit (PWM = f(actualVelocity))")
plt.xlabel("Angezeigte Geschwindigkeit (actualVelocity)")
plt.ylabel("PWM")
plt.title("PWM-Mapping passend zur tatsächlichen Tachoanzeige")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()
