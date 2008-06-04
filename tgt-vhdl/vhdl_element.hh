/*
 *  VHDL abstract syntax elements.
 *
 *  Copyright (C) 2008  Nick Gasson (nick@nickg.me.uk)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef INC_VHDL_ELEMENT_HH
#define INC_VHDL_ELEMENT_HH

#include <fstream>
#include <list>
#include <string>

class vhdl_entity;
class vhdl_arch;

/*
 * Any VHDL syntax element. Each element can also contain a comment.
 */
class vhdl_element {
public:
   virtual ~vhdl_element() {}
   
   virtual void emit(std::ofstream &of, int level=0) const = 0;

   void set_comment(std::string comment);
protected:
   void emit_comment(std::ofstream &of, int level,
                     bool end_of_line=false) const;
private:
   std::string comment_;
};

typedef std::list<vhdl_element*> element_list_t; 

class vhdl_type : public vhdl_element {
public:
   virtual ~vhdl_type() {}
};

/*
 * A type at the moment is just a name. It shouldn't get
 * too much more complex, as Verilog's type system is much
 * simpler than VHDL's.
 */
class vhdl_scalar_type : public vhdl_type {
public:
   vhdl_scalar_type(const char *name) : name_(name) {}

   void emit(std::ofstream &of, int level) const;
private:
   std::string name_;
};

class vhdl_expr : public vhdl_element {
public:
   virtual ~vhdl_expr() {}
};

/*
 * A normal scalar variable reference.
 */
class vhdl_var_ref : public vhdl_expr {
public:
   vhdl_var_ref(const char *name) : name_(name) {}

   void emit(std::ofstream &of, int level) const;
private:
   std::string name_;
};

class vhdl_const_string : public vhdl_expr {
public:
   vhdl_const_string(const char *value) : value_(value) {}

   void emit(std::ofstream &of, int level) const;
private:
   std::string value_;
};

class vhdl_expr_list : public vhdl_element {
public:
   ~vhdl_expr_list();
   
   void emit(std::ofstream &of, int level) const;
   void add_expr(vhdl_expr *e);
private:
   std::list<vhdl_expr*> exprs_;
};


/*
 * A concurrent statement appears in architecture bodies but not
 * processes.
 */
class vhdl_conc_stmt : public vhdl_element {
   friend class vhdl_arch;  // Can set its parent
public:
   vhdl_conc_stmt() : parent_(NULL) {}
   virtual ~vhdl_conc_stmt() {}

   vhdl_arch *get_parent() const;
private:
   vhdl_arch *parent_;
};

typedef std::list<vhdl_conc_stmt*> conc_stmt_list_t;


/*
 * Any sequential statement in a process.
 */
class vhdl_seq_stmt : public vhdl_element {
public:
   virtual ~vhdl_seq_stmt() {}
};

typedef std::list<vhdl_seq_stmt*> seq_stmt_list_t;


/*
 * Delay simulation indefinitely, until an event, or for a
 * specified time.
 */
class vhdl_wait_stmt : public vhdl_seq_stmt {
public:
   void emit(std::ofstream &of, int level) const;
};


/*
 * A procedure call. Which is a statement, unlike a function
 * call which is an expression.
 */
class vhdl_pcall_stmt : public vhdl_seq_stmt {
public:
   vhdl_pcall_stmt(const char *name) : name_(name) {}
   
   void emit(std::ofstream &of, int level) const;
   void add_expr(vhdl_expr *e) { exprs_.add_expr(e); }
private:
   std::string name_;
   vhdl_expr_list exprs_;
};


/*
 * A declaration of some sort (variable, component, etc.).
 * Declarations have names, which is the identifier of the variable,
 * constant, etc. not the type.
 */
class vhdl_decl : public vhdl_element {
public:
   vhdl_decl(const char *name) : name_(name) {}
   virtual ~vhdl_decl() {};

   const std::string &get_name() const { return name_; }
protected:
   std::string name_;
};

typedef std::list<vhdl_decl*> decl_list_t;
   

/*
 * A forward declaration of a component. At the moment it is assumed
 * that components declarations will only ever be for entities
 * generated by this code generator. This is enforced by making the
 * constructor private (use component_decl_for instead).
 */
class vhdl_component_decl : public vhdl_decl {
public:
   virtual ~vhdl_component_decl() {};
   
   static vhdl_component_decl *component_decl_for(const vhdl_entity *ent);

   void emit(std::ofstream &of, int level) const;
private:
   vhdl_component_decl(const char *name);

   // TODO: Ports, etc.
};


/*
 * A variable declaration inside a process (although this isn't
 * enforced here).
 */
class vhdl_var_decl : public vhdl_decl {
public:
   vhdl_var_decl(const char *name, vhdl_type *type)
      : vhdl_decl(name), type_(type) {}
   ~vhdl_var_decl();
   
   void emit(std::ofstream &of, int level) const;
private:
   vhdl_type *type_;
};


/*
 * Instantiation of component. This is really only a placeholder
 * at the moment until the port mappings are worked out.
 */
class vhdl_comp_inst : public vhdl_conc_stmt {
public:
   vhdl_comp_inst(const char *inst_name, const char *comp_name);
   virtual ~vhdl_comp_inst() {}

   void emit(std::ofstream &of, int level) const;
private:
   std::string comp_name_, inst_name_;

   // TODO: Port mappings, etc.
};


/*
 * Container for sequential statements.
 */
class vhdl_process : public vhdl_conc_stmt {
public:
   vhdl_process(const char *name = "");
   virtual ~vhdl_process();

   void emit(std::ofstream &of, int level) const;
   void add_stmt(vhdl_seq_stmt *stmt);
   void add_decl(vhdl_decl *decl);
   bool have_declared_var(const std::string &name) const;
private:
   seq_stmt_list_t stmts_;
   decl_list_t decls_;
   std::string name_;
};


/*
 * An architecture which implements an entity.
 */
class vhdl_arch : public vhdl_element {
   friend class vhdl_entity;  // Can set its parent
public:
   vhdl_arch(const char *entity, const char *name="Behavioural");
   virtual ~vhdl_arch();

   void emit(std::ofstream &of, int level=0) const;
   bool have_declared_component(const std::string &name) const;
   void add_decl(vhdl_decl *decl);
   void add_stmt(vhdl_conc_stmt *stmt);
   vhdl_entity *get_parent() const;
private:
   vhdl_entity *parent_;
   conc_stmt_list_t stmts_;
   decl_list_t decls_;
   std::string name_, entity_;
};

/*
 * An entity defines the ports, parameters, etc. of a module. Each
 * entity is associated with a single architecture (although
 * technically this need not be the case).  Entities are `derived'
 * from instantiations of Verilog module scopes in the hierarchy.
 */
class vhdl_entity : public vhdl_element {
public:
   vhdl_entity(const char *name, const char *derived_from,
               vhdl_arch *arch);
   virtual ~vhdl_entity();

   void emit(std::ofstream &of, int level=0) const;
   vhdl_arch *get_arch() const { return arch_; }
   const std::string &get_name() const { return name_; }
   void requires_package(const char *spec);
   const std::string &get_derived_from() const { return derived_from_; }
private:
   std::string name_;
   vhdl_arch *arch_;  // Entity may only have a single architecture
   std::string derived_from_;
   std::list<std::string> uses_;
};

typedef std::list<vhdl_entity*> entity_list_t;


#endif
