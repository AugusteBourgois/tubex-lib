/** 
 *  Contractor class
 * ----------------------------------------------------------------------------
 *  \date       2020
 *  \author     Simon Rohou
 *  \copyright  Copyright 2020 Simon Rohou
 *  \license    This program is distributed under the terms of
 *              the GNU Lesser General Public License (LGPL).
 */

#include "tubex_Contractor.h"
#include "tubex_CtcEval.h"
#include "tubex_CtcDeriv.h"
#include "tubex_CtcDist.h"

using namespace std;
using namespace ibex;

namespace tubex
{
  int Contractor::ctc_counter = 0;

  Contractor::Contractor(Type type, const vector<Domain*>& v_domains)
    : m_type(type), m_v_domains(v_domains)
  {
    assert(!v_domains.empty());

    ctc_counter++;
    m_ctc_id = ctc_counter;
  }

  Contractor::Contractor(ibex::Ctc& ctc, const vector<Domain*>& v_domains)
    : Contractor(Type::IBEX, v_domains)
  {
    assert(!v_domains.empty());

    m_ibex_ctc = reference_wrapper<ibex::Ctc>(ctc);
  }

  Contractor::Contractor(tubex::Ctc& ctc, const vector<Domain*>& v_domains) 
    : Contractor(Type::TUBEX, v_domains)
  {
    assert(!v_domains.empty());

    m_tubex_ctc = reference_wrapper<tubex::Ctc>(ctc);
    m_tubex_ctc.get().preserve_slicing(true);
  }

  Contractor::Contractor(const Contractor& ac)
    : Contractor(ac.m_type, ac.m_v_domains)
  {
    assert(!ac.m_v_domains.empty());

    m_name = ac.m_name;
    m_ctc_id = ac.m_ctc_id;

    switch(ac.m_type)
    {
      case Type::EQUALITY:
      case Type::COMPONENT:
        // Nothing to do
        break;
        
      case Type::IBEX:
        m_ibex_ctc = reference_wrapper<ibex::Ctc>(ac.m_ibex_ctc);
        break;

      case Type::TUBEX:
        m_tubex_ctc = reference_wrapper<tubex::Ctc>(ac.m_tubex_ctc);
        break;

      default:
        assert(false && "unhandled case");
    }
  }
  
  Contractor::~Contractor()
  {

  }

  int Contractor::id() const
  {
    return m_ctc_id;
  }

  Contractor::Type Contractor::type() const
  {
    return m_type;
  }

  ibex::Ctc& Contractor::ibex_ctc()
  {
    assert(m_type == Type::IBEX);
    return m_ibex_ctc.get();
  }

  tubex::Ctc& Contractor::tubex_ctc()
  {
    assert(m_type == Type::TUBEX);
    return m_tubex_ctc.get();
  }

  bool Contractor::is_active() const
  {
    return m_active;
  }

  void Contractor::set_active(bool active)
  {
    m_active = active;
  }

  vector<Domain*>& Contractor::domains()
  {
    return const_cast<vector<Domain*>&>(static_cast<const Contractor&>(*this).domains());
  }

  const vector<Domain*>& Contractor::domains() const
  {
    return m_v_domains;
  }

  bool Contractor::operator==(const Contractor& x) const
  {
    assert(!m_v_domains.empty() && !x.m_v_domains.empty());

    if(m_type != x.m_type)
      return false;

    switch(m_type)
    {
      case Type::IBEX:
        if(&m_ibex_ctc.get() != &x.m_ibex_ctc.get())
          return false;

        if(typeid(m_ibex_ctc.get()) != typeid(x.m_ibex_ctc.get()))
          return false;

      case Type::TUBEX:
        if(typeid(m_tubex_ctc.get()) != typeid(x.m_tubex_ctc.get()))
          return false;

        if(&m_tubex_ctc.get() != &x.m_tubex_ctc.get() &&
          (typeid(m_tubex_ctc.get()) != typeid(CtcEval) && 
           typeid(m_tubex_ctc.get()) != typeid(CtcDeriv) && 
           typeid(m_tubex_ctc.get()) != typeid(CtcDist)))
          return false;

      case Type::COMPONENT:
      case Type::EQUALITY:
        // Nothing to compare
        break;

      default:
        assert(false && "unhandled case");
        return false;
    }

    if(m_v_domains.size() != x.m_v_domains.size())
      return false;

    for(size_t i = 0 ; i < m_v_domains.size() ; i++)
    {
      bool found = false;
      for(size_t j = 0 ; j < x.m_v_domains.size() ; j++)
        if(*m_v_domains[i] == *x.m_v_domains[j])
        {
          found = true;
          break;
        }

      if(!found)
        return false;
    }

    return true;
  }

  void Contractor::contract()
  {
    assert(!m_v_domains.empty());

    if(m_type == Type::IBEX)
    {
      // Data may be presented in two ways:
      // - all components in one vector box
      // - a list of heterogeneous components

      // Case: all components in one vector box
      if(m_v_domains.size() == 1 && m_v_domains[0]->type() == Domain::Type::INTERVAL_VECTOR)
      {
        m_ibex_ctc.get().contract(m_v_domains[0]->interval_vector());
      }

      // Case: list of heterogeneous components
      else
      {
        for(int j = 0 ; j < 3 ; j++) // to possibly deal with 3 subdomains of a Slice (gates + envelope)
        {
          bool at_least_one_slice = false;
          // if this ^ stays false, then the for loop will break after the first iteration

          // Building a temporary box for the contraction
          
            IntervalVector box(m_ibex_ctc.get().nb_var);

            int i = 0;
            for(auto& dom : m_v_domains)
            {
              switch(dom->type())
              {
                case Domain::Type::INTERVAL:
                  box[i] = dom->interval();
                  i++;
                  break;

                case Domain::Type::INTERVAL_VECTOR:
                  assert(false && "interval vectors should not be handled here");
                  box.put(i, dom->interval_vector());
                  i+=dom->interval_vector().size();
                  break;

                case Domain::Type::SLICE:
                  switch(j)
                  {
                    case 0: // we start from the envelope
                      box[i] = dom->slice().codomain();
                      break;
                    
                    // Then the gates
                    case 1:
                      box[i] = dom->slice().input_gate();
                      break;
                      
                    case 2:
                      box[i] = dom->slice().output_gate();
                      break;

                    default:
                      assert(false && "Slice domain already treated");
                  }
                  i++;
                  at_least_one_slice = true;
                  break;

                case Domain::Type::TUBE:
                case Domain::Type::TUBE_VECTOR:
                  assert(false && "dynamic domains should not be handled here");
                  break;

                default:
                  assert(false && "unhandled case");
              }
            }

            assert(i == m_ibex_ctc.get().nb_var);

          // Contracting

            m_ibex_ctc.get().contract(box);
            
          // Updating the domains (reverse operation)

            i = 0;
            for(auto& dom : m_v_domains)
            {
              switch(dom->type())
              {
                case Domain::Type::INTERVAL:
                {
                  dom->interval() = box[i];
                  i++;
                }
                break;

                case Domain::Type::INTERVAL_VECTOR:
                {
                  int vector_size = dom->interval_vector().size();
                  dom->interval_vector() = box.subvector(i, i+vector_size);
                  i+=vector_size;
                }
                break;

                case Domain::Type::SLICE:
                {
                  switch(j)
                  {
                    case 0:
                      dom->slice().set_envelope(box[i]);
                      break;

                    case 1:
                      dom->slice().set_input_gate(box[i]);
                      break;

                    case 2:
                      dom->slice().set_output_gate(box[i]);
                      break;

                    default:
                      assert(false && "Slice domain already treated");
                  }
                  i++;
                }
                break;

                default:
                  assert(false && "unhandled case");
              }
            }

            assert(i == m_ibex_ctc.get().nb_var);

          if(!at_least_one_slice)
            break;
        }
      }
    }

    else if(m_type == Type::TUBEX)
    {
      m_tubex_ctc.get().contract(m_v_domains);
    }

    else if(m_type == Type::COMPONENT)
    {
      // Symbolic
    }

    else if(m_type == Type::EQUALITY)
    {
      assert(m_v_domains.size() == 2);
      assert(m_v_domains[0]->type() == m_v_domains[1]->type());

      switch(m_v_domains[0]->type())
      {
        case Domain::Type::INTERVAL:
        {
          Interval inter = m_v_domains[0]->interval() & m_v_domains[1]->interval();
          m_v_domains[0]->interval() = inter;
          m_v_domains[1]->interval() = inter;
        }
        break;
      
        case Domain::Type::INTERVAL_VECTOR:
        {
          IntervalVector inter = m_v_domains[0]->interval_vector() & m_v_domains[1]->interval_vector();
          m_v_domains[0]->interval_vector() = inter;
          m_v_domains[1]->interval_vector() = inter;
        }
        break;
    
        case Domain::Type::TUBE:
        {
          Tube inter = m_v_domains[0]->tube() & m_v_domains[1]->tube();
          m_v_domains[0]->tube() = inter;
          m_v_domains[1]->tube() = inter;
        }
        break;
        
        case Domain::Type::TUBE_VECTOR:
        {
          TubeVector inter = m_v_domains[0]->tube_vector() & m_v_domains[1]->tube_vector();
          m_v_domains[0]->tube_vector() = inter;
          m_v_domains[1]->tube_vector() = inter;
        }
        break;

        default:
          assert(false && "unhandled case");
      }
    }

    else
      assert(false && "unhandled case");
  }
  
  const string Contractor::name() const
  {
    switch(type())
    {
      case Type::COMPONENT:
        return "";

      case Type::EQUALITY:
        return "=";

      case Type::TUBEX:
        if(m_name.empty())
        {
          if(typeid(m_tubex_ctc.get()) == typeid(CtcEval))
            return "\\mathcal{C}_{\\textrm{eval}}";

          if(typeid(m_tubex_ctc.get()) == typeid(CtcDeriv))
            return "\\mathcal{C}_{\\frac{d}{dt}}";
        }
        return "\\mathcal{C}_{" + m_name + "}";

      case Type::IBEX:
      default:
        return "\\mathcal{C}_{" + m_name + "}";
    }
  }
  
  void Contractor::set_name(const string& name)
  {
    m_name = name;
  }
  
  ostream& operator<<(ostream& str, const Contractor& x)
  {
    str << "Contractor " << x.name() << " (" << x.m_v_domains.size() << " doms)" << flush;
    return str;
  }
}