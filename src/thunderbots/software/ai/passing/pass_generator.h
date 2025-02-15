#pragma once

#include <mutex>
#include <random>
#include <thread>

#include "ai/passing/pass.h"
#include "ai/world/world.h"
#include "util/gradient_descent.h"
#include "util/parameter/dynamic_parameters.h"
#include "util/time/timestamp.h"

namespace Passing
{
    /**
     * This class is responsible for generating passes for us to perform
     *
     * == General Description ==
     * It is built such that when it is constructed, a thread is immediately started in
     * the background continuously optimizing/pruning/re-generating passes. As such, any
     * modifications to data in this class through a public interface *must* be managed
     * by mutexes. Generally in functions that touch data we use "std::lock_guard" to
     * take ownership of the mutex protecting that data for the duration of the function.
     *
     * == Making Changes/Additions ==
     * Whenever you change/add a function, you need to ask: "what data is this _directly_
     * touching?". If the function is touching anything internal to the class, you should
     * be getting a lock on the mutex for that data member for the duration of the
     * function (see below for examples). Note the *directly* bit! If you are
     * changing/adding function "A", and you have it call function "B", if B touches
     * data, then B should be responsible for getting a lock on the data. If you acquire
     * a lock in A, then call B, which also requires a lock, then the threads will
     * deadlock and everything will grind to a halt.
     *
     * == Performance Considerations ==
     * It is important that the pass generator is able to "keep up" with the current
     * time. This is because a pass is defined in part by it's start time. If the
     * pass generator only completes an interation of pass updates once every 5 seconds,
     * then the start times for the passes will be in the past (if we choose to pass
     * very soon when optimizing the pass), and so the passes will likely be invalid by
     * the time another iteration starts. Because of this, it isextremely important that
     * the pass generator runs fast enough. Debug builds running on slightly slower
     * computers could be unable to converge. It is recommended that all testing of things
     * involving the PassGenerator be done with executables built in "Release" in order to
     * maximize performance ("Release" can be 2-10x faster then "Debug").
     */
    class PassGenerator
    {
       public:
        // Delete the default constructor, we want to force users to choose what
        // pass quality they deem reasonable
        PassGenerator() = delete;

        // Delete the copy and assignment operators because this class really shouldn't
        // need them and we don't want to risk doing anything nasty with the internal
        // multithreading this class uses
        PassGenerator& operator=(const PassGenerator&) = delete;
        PassGenerator(const PassGenerator&)            = delete;

        /**
         * Create a PassGenerator with given parameters
         *
         * @param world The world we're passing int
         * @param passer_point The point we're passing from
         */
        explicit PassGenerator(const World& world, const Point& passer_point);

        /**
         * Updates the world
         *
         * @param world
         */
        void setWorld(World world);

        /**
         * Updates the point that we are passing from
         *
         * @param passer_point the point we are passing from
         */
        void setPasserPoint(Point passer_point);

        /**
         * Set the id of the robot performing the pass.
         *
         * This id will be used so we ignore the passer when determining where to
         * pass to. We assume this robot is on the friendly team.
         *
         * @param robot_id The id of the robot performing the pass
         */
        void setPasserRobotId(unsigned int robot_id);

        /**
         * Set the target region that we would like to pass to
         *
         * @param area An optional that may contain the area to pass to. If the
         *             optional is empty (ie. `std::nullopt`) then this indicates
         *             that there is no target region
         */
        void setTargetRegion(std::optional<Rectangle> area);

        /**
         * Gets the best pass we know of so far
         *
         * This only returns what we know so far. For example, if called directly after
         * the world state is updated, it is unlikely to return good results (if any).
         * Gradient descent must be allowed to run for some number of iterations before
         * this can be used to get a reasonable value.
         *
         * @return The best currently known pass and the rating of that pass (in [0-1])
         */
        std::pair<Pass, double> getBestPassSoFar();

        /**
         * Destructs this PassGenerator
         */
        ~PassGenerator();


       private:
        // The number of parameters (representing a pass) that we optimize
        // (pass_start_x, pass_start_y, pass_speed, pass_start_time)
        static const int NUM_PARAMS_TO_OPTIMIZE = 4;


        // Weights used to normalize the parameters that we pass to GradientDescent
        // (see the GradientDescent documentation for details)
        // These weights are *very* roughly the step that gradient descent will take
        // in each respective dimension for a single iteration. They are tuned to
        // ensure passes converge as fast as possible, but are also as stable as
        // possible
        static constexpr double PASS_SPACE_WEIGHT                          = 0.1;
        static constexpr double PASS_TIME_WEIGHT                           = 0.1;
        static constexpr double PASS_SPEED_WEIGHT                          = 0.01;
        std::array<double, NUM_PARAMS_TO_OPTIMIZE> optimizer_param_weights = {
            PASS_SPACE_WEIGHT, PASS_SPACE_WEIGHT, PASS_TIME_WEIGHT, PASS_SPEED_WEIGHT};

        /**
         * Continuously optimizes, prunes, and re-generates passes based on known info
         *
         * This will only return when the in_destructor flag is set
         */
        void continuouslyGeneratePasses();

        /**
         * Optimizes all current passes
         */
        void optimizePasses();

        /**
         * Prunes un-promising passes and replaces them with newly generated ones
         */
        void pruneAndReplacePasses();

        /**
         * Saves the best currently known pass
         */
        void saveBestPass();

        /**
         * Draws all the passes we are currently optimizing and the gradient of pass
         * receive position quality over the field
         */
        void visualizePassesAndPassQualityGradient();

        /**
         * Get the number of passes to keep after pruning
         *
         * @return the number of passes to keep after pruning
         */
        unsigned int getNumPassesToKeepAfterPruning();

        /**
         * Get the number of passes to optimize
         *
         * @return the number of passes to optimize
         */
        unsigned int getNumPassesToOptimize();

        /**
         * Convert the given pass to an array
         *
         * @param pass The pass to convert
         *
         * @return An array containing the parts of the pass we want to optimize, in the
         *         form: {receiver_point.x, receiver_point.y, pass_speed_m_per_s
         *                pass_start_time}
         */
        std::array<double, NUM_PARAMS_TO_OPTIMIZE> convertPassToArray(Pass pass);

        /**
         * Convert a given array to a Pass
         *
         * @param array The array to convert to a pass, in the form:
         *              {receiver_point.x, receiver_point.y, pass_speed_m_per_s,
         *              pass_start_time}
         *
         * @return The pass represented by the given array, with the passer point being
         *         the current `passer_point` we're optimizing for
         */
        Pass convertArrayToPass(std::array<double, NUM_PARAMS_TO_OPTIMIZE> array);

        /**
         * Calculate the quality of a given pass
         * @param pass The pass to rate
         * @return A value in [0,1] representing the quality of the pass with 1 being the
         *         best pass and 0 being the worst pass
         */
        double ratePass(Pass pass);

        /**
         * Updates the passer point of all passes that we're currently optimizing
         *
         * @param new_passer_point The new passer point
         */
        void updatePasserPointOfAllPasses(const Point& new_passer_point);

        /**
         * Compares the quality of the two given passes
         *
         * @param pass1
         * @param pass2
         *
         * @return pass1.quality < pass2.quality
         */
        bool comparePassQuality(const Pass& pass1, const Pass& pass2);

        /**
         * Check if the two given passes are equal
         *
         * Equality here is defined in the context of this class, in that we use it as a
         * measure of whether or not to merge two passes
         *
         * @param pass1
         * @param pass2
         *
         * @return True if the two passes are similar enough to be equal, false otherwise
         */
        bool passesEqual(Pass pass1, Pass pass2);

        /**
         * Generate a given number of passes
         *
         * This function is used to generate the initial passes that are then optimized
         * via gradient descent.
         *
         * @param num_passes_to_gen  The number of passes to generate
         *
         * @return A vector containing the requested number of passes
         */
        std::vector<Pass> generatePasses(unsigned long num_passes_to_gen);

        // The thread running the pass optimization/pruning/re-generation in the
        // background. This thread will run for the entire lifetime of the class
        std::thread pass_generation_thread;

        // The mutex for the in_destructor flag
        std::mutex in_destructor_mutex;

        // This flag is used to indicate that we are in the destructor. We use this to
        // communicate with pass_generation_thread that it is
        // time to stop
        bool in_destructor;

        // The mutex for the world
        std::mutex world_mutex;

        // This world is what is used in the optimization loop
        World world;

        // The mutex for the updated world
        std::mutex updated_world_mutex;

        // This world is the most recently updated one. We use this variable to "buffer"
        // the most recently updated world so that the world stays the same for the
        // entirety of each optimization loop, which makes things easier to reason about
        World updated_world;

        // The mutex for the passer_point
        std::mutex passer_point_mutex;

        // The point we are passing from
        Point passer_point;

        // The mutex for the passer robot ID
        std::mutex passer_robot_id_mutex;

        // The id of the robot that is performing the pass. We want to ignore this robot
        std::optional<unsigned int> passer_robot_id;

        // The mutex for the target region
        std::mutex target_region_mutex;

        // The area that we want to pass to
        std::optional<Rectangle> target_region;

        // The mutex for the passer_point
        std::mutex best_known_pass_mutex;

        // The best pass we currently know about
        Pass best_known_pass;

        // All the passes that we are currently trying to optimize in gradient descent
        std::vector<Pass> passes_to_optimize;

        // The optimizer we're using to find passes
        Util::GradientDescentOptimizer<NUM_PARAMS_TO_OPTIMIZE> optimizer;

        // A random number generator for use across the class
        std::random_device random_device;
        std::mt19937 random_num_gen;
    };


}  // namespace Passing
