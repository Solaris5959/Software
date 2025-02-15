#pragma once

#include <string>
#include <vector>

#include "ai/intent/avoid_area.h"

// We forward-declare the IntentVisitor interface (pure virtual class) because we need
// to know about the existence of this class in order to accept visitors with the
// accept() function. We cannot use an #include statement because this creates a cyclic
// dependency
//
// This class can be found in ai/intent/visitor/intent_visitor.h
class IntentVisitor;

/**
 * An intent is a simple "thing" a robot or player may want to do. It specifies WHAT a
 * robot should do, not necessarily exactly how it will do it. Examples are shooting at
 * a point (for example the enemy net), or moving to a location.
 *
 * Intents can be considered to be one level above Primitives in terms of abstraction.
 * Primitives are simply the smallest/simplest action a robot can take and are not
 * concerned with gameplay logic, while Intents do deal with gameplay logic.
 *
 * We define an Abstract base class for Intents, despite not providing very many
 * pure-virtual functions, so that we can create generic structures of Intents.
 * For example we can create vectors of generic Intent objects (using pointers)
 * which is easier than a separate container for each type of Intent.
 */
class Intent
{
   public:
    /**
     * Creates a new Intent with the given priority. A larger number indicates a higher
     * priority. The priority value must be in the range [0, 100]
     *
     * @param priority The priority of this Intent
     */
    explicit Intent(unsigned int priority);

    /**
     * Returns the name of this Intent
     *
     * @return the name of this Intent
     */
    virtual std::string getIntentName(void) const = 0;

    /**
     * Returns the priority of this Intent. The priority value is an integer in the range
     * [0, 100] that indicates the priority of this Intent.
     *
     * @return the priority of this Intent
     */
    unsigned int getPriority(void) const;

    /**
     * Sets the priority of this Intent. The priority value must be an integer in the
     * range [0, 100]
     */
    void setPriority(unsigned int new_priority);

    /**
     * Compares Intents for equality. Intents are considered equal if all
     * their member variables are equal.
     *
     * @param other the Intents to compare with for equality
     * @return true if the Intents are equal and false otherwise
     */
    bool operator==(const Intent& other) const;

    /**
     * Compares Intents for inequality.
     *
     * @param other the Intent to compare with for inequality
     * @return true if the Intents are not equal and false otherwise
     */
    bool operator!=(const Intent& other) const;

    /**
     * Accepts an Intent Visitor and calls the visit function
     *
     * @param visitor An Intent Visitor
     */
    virtual void accept(IntentVisitor& visitor) const = 0;

    /**
     * Get the areas this intent should avoid moving into
     *
     * @return The areas this intent should avoid moving into
     */
    std::vector<AvoidArea> getAreasToAvoid() const;

    /**
     * Set the areas this intent should avoid moving into
     *
     * @param areas_to_avoid The areas this intent should avoid moving into
     */
    void setAreasToAvoid(const std::vector<AvoidArea>& areas_to_avoid);

    virtual ~Intent() = default;

   private:
    /**
     * The priority of this intent. Must be in the range [0, 100]
     * higher value => higher priority
     */
    unsigned int priority;

    /**
     * The areas this intent should avoid moving into. These are enforced by
     * the navigator
     */
    std::vector<AvoidArea> areas_to_avoid;
};
