
Outdoor renderer:

Volumetric Light Scattering as a Post-Process
Kenny Mitchell
Electronic Arts

shadow mapping for the sun

exhaust plumes rendered as meshes with fuzzy edges


Indoor renderer:

some kind of spatial tree structure, no sun or shadow map but many light sources
uniform grid on gpu to simulate gas and smoke?

one mesh per room and one per recently moved object?

flood fill algorithm for lighting?






----


render a light volume for each light source
sphere, cone or frustum, accumulating hits to a depth buffer

in indoor environments the number of objects to render is small and we will render them at resolutions dependent on
the distance to the camera.



