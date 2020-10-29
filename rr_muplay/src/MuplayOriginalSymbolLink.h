/* This represents a link from the modified binary back to the original symbol */
#ifndef RR_MUPLAY_ORIGINAL_SYMBOL_LINK_
#define RR_MUPLAY_ORIGINAL_SYMBOL_LINK_
#include "chunk/link.h"

namespace rr {

    class MuplayOriginalSymbolLinkBase : public LinkScopeDecorator<
    Link::SCOPE_UNKNOWN, LinkDefaultAttributeDecorator<Link>> {
    private:
        /* The symbol from the original exe */
        Symbol *symbol;
        /* Address of the link target in the old exe */
        address_t target;
        /* Offset after the symbol in the old exe */
        address_t offset;
    public:
      MuplayOriginalSymbolLinkBase(Symbol* symbol, address_t target, address_t offset)
        : symbol(symbol), target(target), offset(offset) {}
        /* Is it RIP relative or not (yes is text, no is data) */
        virtual bool isRIPRelative() const { return true; }

        virtual ChunkRef getTarget() const { return nullptr; }
        virtual address_t getTargetAddress() const { return target; }
    };

    class MuplayOriginalSymbolLinkText : public MuplayOriginalSymbolLinkBase {
    public:
      MuplayOriginalSymbolLinkText(Symbol* symbol, address_t target, address_t offset) :
      MuplayOriginalSymbolLinkBase(symbol, target, offset) { }
      virtual bool isRIPRelative() const { return true; }
    };

    class MuplayOriginalSymbolLinkData : public MuplayOriginalSymbolLinkBase {
    public:
      using MuplayOriginalSymbolLinkBase::MuplayOriginalSymbolLinkBase;
      virtual bool isRIPRelative() const { return false; }
    };

} //namespace rr
#endif
