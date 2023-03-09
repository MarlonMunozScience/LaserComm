# How to calculate Poisson Probability from spreadsheet

From the master noise data sheet, we are given the frequency of noise is different units. If we can assume that the receiver is diffraction limited, then we are given the noise in the units of $\frac{photons}{seconds  * microns} $

Given the slot time and a chosen wavelength to transmit the photons, we can easily calculate the expected photons for each noise source.

$$ E(X) = \lambda = Noise * s * \mu m $$

Where $s$ is equal to the length of the slot in the PPM frame and $\mu m$ is the wavelength of the incident photons.

Now that we have a value for $\lambda $, we can calculate the probability of getting noise in a PPM slot. The formula for a poisson distribution is:

$$ Pr(X=k)  = \frac{\lambda^{k}e^{-\lambda}}{k!} $$

If we choose to only care about any photons being in the slot, with more than one photon in a slot not being relevant, we can calculate the probabilities based on the fact that 
$$\sum_{X=0}^{\inf} Pr(X) = 1 \quad  \rightarrow \quad  Pr(X \geq 1) = 1 - P(X = 0) $$
