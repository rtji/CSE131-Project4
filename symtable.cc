#include "symtable.h"

using namespace std;

ScopedTable :: ScopedTable() {
}

ScopedTable :: ~ScopedTable() {}

void ScopedTable :: insert (Symbol &sym) {
  symbols.insert(pair<const char *, Symbol>(sym.name, sym));
}

void ScopedTable :: remove (Symbol &sym) {
  symbols.erase(sym.name);
}

Symbol* ScopedTable :: find (const char *name) {
  // cerr << "from map " << symbols->find(name) << endl;
  // return &symbols.find(name)->second;

  SymbolIterator it;

  it = symbols.find(name);

  if (it == symbols.end()) {
    return NULL;
  }
  else {
    return &symbols.find(name)->second;
  }

/*
  if (found == symbols.end()) {
    return NULL;
  }
  else {
    return found;
  }
  */
}

/*
void ScopedTable::printScopedTable() {
  SymbolIterator it = symbols.begin();
  for(; it != symbols.end(); it++){
    if(it->second.kind == 1){
      cerr << "  Name: " << (it)->second.decl << endl;
      cerr << "  Kind: VarDecl" << endl;
      // cerr << "  Type: " << (it)->second.decl->GetType()->PrintToStream() << endl <<
      cerr << "  someInfo: "  << (it)->second.someInfo << endl;
    }
    else {
      cerr << (it)->second.decl << endl << "  "  << "Kind: " << "FnDecl" <<
      endl << "  " << "someInfo: "  << (it)->second.someInfo << endl;
    }
  }
}

void SymbolTable::printSymbolTable(bool debug) {

  if (!debug) return;

  cerr << "\n=============== PRINTING SYMBOL TABLE ===============\n";
  for (int i = 0; i < tables.size(); i++) {
    ScopedTable *current = tables.at(i);
    cerr << "LEVEL " << i << "" << endl;
    current->printScopedTable();
  }
  cerr << "================= END SYMBOL TABLE =================\n\n\n";

  // ScopedTable *current = tables.at(tables.size() - 1);
  // tables.at(tables.size() - 1).printScopedTable();
}

*/
SymbolTable :: SymbolTable () {
  // tables = {};
  tables.push_back(new ScopedTable());
}

SymbolTable :: ~SymbolTable () {}

void SymbolTable :: push () {
  tables.push_back(new ScopedTable());
}

void SymbolTable :: pop () {
  tables.pop_back();
}

void SymbolTable :: insert (Symbol &sym) {
  ScopedTable *current = tables.at(tables.size() - 1);
  current->insert(sym);
}

void SymbolTable :: remove (Symbol & sym) {
  ScopedTable *current = tables.at(tables.size() - 1);
  current->remove(sym);
}

Symbol* SymbolTable :: find (const char *name) {

  for (int i = tables.size() - 1; i >= 0; i--) {
    ScopedTable *current = tables.at(i);
    Symbol *result = current->find(name);
    if (result) {
      return result;
    }
  }

  return NULL;
}

/*
Symbol* SymbolTable :: findInCurrentScope (const char *name) {
	ScopedTable *current = tables.at(tables.size()-1);
	return current->find(name);
}
*/ 
