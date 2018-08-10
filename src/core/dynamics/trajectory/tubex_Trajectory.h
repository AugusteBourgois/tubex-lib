/* ============================================================================
 *  tubex-lib - Trajectory class
 * ============================================================================
 *  Copyright : Copyright 2017 Simon Rohou
 *  License   : This program is distributed under the terms of
 *              the GNU Lesser General Public License (LGPL).
 *
 *  Author(s) : Simon Rohou
 *  Bug fixes : -
 *  Created   : 2018
 * ---------------------------------------------------------------------------- */

#ifndef __TUBEX_TRAJECTORY_H__
#define __TUBEX_TRAJECTORY_H__

#include <map>
#include "tubex_DynamicalItem.h"
#include "tubex_Function.h"

namespace tubex
{
  class Trajectory : public DynamicalItem
  {
    public:

      // Definition
      Trajectory();
      Trajectory(const ibex::Interval& domain, const tubex::Function& f, int f_image_id = 0);
      Trajectory(const std::map<double,double>& m_map_values);
      Trajectory(const Trajectory& traj);
      ~Trajectory();
      const Trajectory& operator=(const Trajectory& x);
      int size() const;

      // Access values
      const std::map<double,double>& get_map() const;
      const tubex::Function* get_function() const;
      const ibex::Interval domain() const;
      const ibex::Interval codomain() const;
      double operator()(double t) const;
      const ibex::Interval operator()(const ibex::Interval& t) const;

      // Tests
      bool not_defined() const;
      bool operator==(const Trajectory& x) const;
      bool operator!=(const Trajectory& x) const;

      // Setting values
      void set(double t, double y);
      void truncate_domain(const ibex::Interval& domain);
      void shift_domain(double shift_ref);

      // String
      friend std::ostream& operator<<(std::ostream& str, const Trajectory& x);
      const std::string class_name() const { return "Trajectory"; };

    protected:

      const ibex::IntervalVector codomain_box() const;

      /** Class variables **/

        ibex::Interval m_domain = ibex::Interval::EMPTY_SET;
        ibex::Interval m_codomain = ibex::Interval::EMPTY_SET;
        // A trajectory is defined either by a Function or a map of values
        tubex::Function *m_function = NULL;
        std::map<double,double> m_map_values;
  };
}

#endif