#include <vector>
#include <cmath>

// include libFastNoise.a
// it would be cool to use terragen from planetside.co.uk but I want something fast and lightweight that can
// run on each player's computer more than I want really beautiful and detailed planets

struct polar_vert {
    double lat;
    double lon;
    double elevation;
};

double lowFrequencyNoise(double lat, double lon) {
    // Placeholder: replace with actual noise function
    return sin(lat * 0.0174533) * cos(lon * 0.0174533); // Simple sine-cosine function for illustration
}

double highFrequencyNoise(double lat, double lon) {
    // Placeholder: replace with actual noise function
    return cos(lat * 0.0174533 * 5) * sin(lon * 0.0174533 * 5); // Higher frequency sine-cosine
}

std::vector<polar_vert> distributePointsOnSphere(int num_points) {
    std::vector<polar_vert> points;
    double phi = M_PI * (3. - sqrt(5.)); // Golden angle in radians

    for (int i = 0; i < num_points; ++i) {
        double y = 1 - (i / (double)(num_points - 1)) * 2; // y goes from 1 to -1
        double radius = sqrt(1 - y * y); // radius at y

        double theta = phi * i; // golden angle increment

        double x = cos(theta) * radius;
        double z = sin(theta) * radius;

        // Convert from Cartesian to spherical coordinates
        double lat = asin(y);
        double lon = atan2(z, x);

        points.push_back({lat * (180/M_PI), lon * (180/M_PI), 0.0}); // Convert to degrees
    }

    return points;
}

std::vector<polar_vert> generateTerrain(int num_points) {
    std::vector<polar_vert> vertices;

    // Distribute points using your chosen method
    auto points = distributePointsOnSphere(num_points);

    // Step 2: Assign base elevations with low-frequency noise
    for (auto& point : points) {
        point.elevation = lowFrequencyNoise(point.lat, point.lon);
    }

    // Step 4: Refine with high-frequency noise
    for (auto& point : points) {
        point.elevation += highFrequencyNoise(point.lat, point.lon);
    }

    // Convert to polar_vert and classify features
    for (auto& point : points) {
        polar_vert pv = {point.lat, point.lon, point.elevation};
        // Optionally classify point as peak, ridge, or valley here
        vertices.push_back(pv);
    }

    return vertices;
}


