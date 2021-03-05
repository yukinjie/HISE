/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


namespace snex {
namespace cppgen {
using namespace juce;

namespace FactoryIds
{
#define DECLARE_ID(x) static const Identifier x(#x);

	DECLARE_ID(container);
	DECLARE_ID(core);
	DECLARE_ID(wrap);
	DECLARE_ID(parameter);

#undef  DECLARE_ID

	static bool isContainer(const NamespacedIdentifier& id)
	{
		return id.getParent().getIdentifier() == container;
	}

	static bool isParameter(const NamespacedIdentifier& id)
	{
		return id.getParent().getIdentifier() == parameter;
	}
};

void ValueTreeBuilder::setHeaderForFormat()
{
	switch (outputFormat)
	{
	case Format::TestCaseFile:
	{
		JitFileTestCase::HeaderBuilder hb(v);
		setHeader(hb);
		break;
	}
	case Format::JitCompiledInstance:
	{
#if 0
		HeaderBuilder hb("My Funky Node");

		String l1, l2, l3;

		l1 << "Author:" << AlignMarker << "Funkymaster";
		l2 << "Creation Date:" << AlignMarker << Time::getCurrentTime().toString(true, false, false);
		l3 << "Project:" << AlignMarker << "ProjectName";

		hb << l1 << l2 << l3;

		hb.addEmptyLine();
		hb << "This code is autogenerated and will be overwritten the next time";
		hb << "you change the scriptnode patch";
#endif

		setHeader([](){ return "/* Autogenerated code */"; });
		break;
	}
	case Format::CppDynamicLibrary:
	{

	}
	}

}

void ValueTreeBuilder::rebuild()
{
	clear();

	return;

	try
	{
		Sanitizers::setChannelAmount(v);
		
		{
			*this << getGlueCode(FormatGlueCode::PreNamespaceCode);

			Namespace impl(*this, getGlueCode(FormatGlueCode::WrappedNamespace), false);
			addComment("Node & Parameter type declarations", Base::CommentType::FillTo80);
			pooledTypeDefinitions.add(parseNode(v));
			impl.flushIfNot();

			getGlueCode(FormatGlueCode::PublicDefinition);
		}
	}
	catch (Error& e)
	{
		r = Result::fail(e.errorMessage);
		wrongNodeCode = e.toString();
	}
}

String ValueTreeBuilder::getGlueCode(ValueTreeBuilder::FormatGlueCode c)
{
	if (outputFormat == Format::TestCaseFile)
	{
		switch (c)
		{
		case FormatGlueCode::WrappedNamespace: return "impl";
		case FormatGlueCode::PublicDefinition:
		{
			UsingTemplate instance(*this, "processor", NamespacedIdentifier::fromString("wrap::node"));

			instance << *pooledTypeDefinitions.getLast();
			instance.flushIfNot();
			return {};
		}
		case FormatGlueCode::MainInstanceClass: return getNodeId(v).getIdentifier().toString();
		}
	}

	if(outputFormat == Format::JitCompiledInstance)
	{
		switch (c)
		{
			case FormatGlueCode::WrappedNamespace: return "impl";
			case FormatGlueCode::PublicDefinition:
			{
				UsingTemplate instance(*this, v[PropertyIds::ID].toString(), NamespacedIdentifier::fromString("wrap::node"));

				instance << *pooledTypeDefinitions.getLast();
				instance.flushIfNot();
				return {};
			}
			case FormatGlueCode::MainInstanceClass: return getNodeId(v).getIdentifier().toString();
		}
	}

	if (outputFormat == Format::CppDynamicLibrary)
	{
		switch (c)
		{
		case FormatGlueCode::PreNamespaceCode: 
		{
			*this << "#pragma once";
			addEmptyLine();


			Include(*this, "JuceHeader.h");
			UsingNamespace(*this, NamespacedIdentifier("scriptnode"));
			UsingNamespace(*this, NamespacedIdentifier("snex"));
			UsingNamespace(*this, NamespacedIdentifier("snex").getChildId("Types"));
			
			return {};
		}
		case FormatGlueCode::WrappedNamespace: return v[PropertyIds::ID].toString() + "_impl";
		case FormatGlueCode::MainInstanceClass: return "instance";
		case FormatGlueCode::PublicDefinition: 
		{
			addComment("Public Definition", Base::CommentType::FillTo80);

			Namespace n(*this, "project", false);

			UsingTemplate instance(*this, v[PropertyIds::ID].toString(), NamespacedIdentifier::fromString("wrap::node"));
			instance << *pooledTypeDefinitions.getLast();
			instance.flushIfNot();
			return {};
		}
		
		}
	}

	return {};
}

void ValueTreeBuilder::addNodeComment(Node::Ptr n)
{
	auto nodeComment = n->nodeTree[PropertyIds::Comment].toString();

	if (nodeComment.isNotEmpty())
	{
		nodeComment = nodeComment.upToFirstOccurrenceOf("\n", false, false);
		addComment(nodeComment.trim(), Base::CommentType::FillTo80Light);
		n->flushIfNot();
	}
}

Node::Ptr ValueTreeBuilder::parseFixChannel(const ValueTree& n)
{
	auto u = getNode(n, false);

	auto wf = createNode(n, {}, "wrap::fix");

	*wf << (int)n[PropertyIds::NumChannels];
	*wf << *u;

	return wf;
}

Node::Ptr ValueTreeBuilder::getNode(const ValueTree& n, bool allowZeroMatch)
{
	auto id = getNodeId(n);

	if (auto existing = getTypeDefinition(id))
	{
		return existing;
	}
	else
	{
		

		if (allowZeroMatch)
			return nullptr;
		else
		{
			Error e;
			e.v = n;
			e.errorMessage = "Can't find node";
			throw e;

			RETURN_IF_NO_THROW(nullptr);
		}
	}
}

Node::Ptr ValueTreeBuilder::getNode(const NamespacedIdentifier& id, bool allowZeroMatch) const
{
	for (auto n : pooledTypeDefinitions)
	{
		if (n->scopedId == id)
			return n;
	}

	if (allowZeroMatch)
		return nullptr;

	Error e;
	e.errorMessage = "Can't find node " + id.toString();
	throw e;

	RETURN_IF_NO_THROW(nullptr);
}



Node::Ptr ValueTreeBuilder::parseNode(const ValueTree& n)
{
	if (auto existing = getNode(n, true))
		return existing;

	auto typeId = getNodeId(n);

	Node::Ptr newNode = createNode(n, typeId.getIdentifier(), getNodePath(n).toString());

	return parseRoutingNode(newNode);
}

Node::Ptr ValueTreeBuilder::parseRoutingNode(Node::Ptr u)
{
	if (getNodePath(u->nodeTree).getParent().toString() == "routing")
	{
		PooledCableType::TemplateConstants c;

		

		c.numChannels = (int)u->nodeTree[PropertyIds::NumChannels];
		c.isFrame = ValueTreeIterator::forEachParent(u->nodeTree, [&](ValueTree& v)
		{
			if (v.getType() == PropertyIds::Node)
			{
				if (v[scriptnode::PropertyIds::FactoryPath].toString().contains("frame"))
					return true;
			}

			return false;
		});


		PooledCableType::Ptr nc;

		if (auto existing = pooledCables.getExisting(c))
		{
			nc = existing;
		}
		else
		{
			String s;

			if (c.numChannels == 1)
				s << "mono";
			else if (c.numChannels == 2)
				s << "stereo";
			else
				s << "multi" << c.numChannels;

			if (c.isFrame)
				s << "_frame";

			s << "_cable";

			nc = new PooledCableType(*this, s, c);
			
			nc->flushIfNot();

			pooledCables.add(nc);
		}

		*u << nc->toExpression();

	}

	return parseOptionalSnexNode(u);
}



Node::Ptr ValueTreeBuilder::parseOptionalSnexNode(Node::Ptr u)
{
	auto p = getNodePath(u->nodeTree).toString();

	if (p.endsWith("midi"))
	{
		String type = ValueTreeIterator::getNodeProperty(u->nodeTree, PropertyIds::Mode).toString().toLowerCase();

		if (type == "custom")
		{
			return parseSnexNode(u);
		}

		Node::Ptr wn = new Node(*this, u->scopedId.id, NamespacedIdentifier(p));

		wn->nodeTree = u->nodeTree;
		String subName = "midi_logic::" + type;

		*wn << subName;

		return parseMod(wn);
	}

	return parseContainer(u);
}


Node::Ptr ValueTreeBuilder::parseSnexNode(Node::Ptr u)
{
	auto p = getNodePath(u->nodeTree).toString();
	auto code = ValueTreeIterator::getSnexCode(u->nodeTree);

	if (code.isNotEmpty())
	{
		String comment;

		comment << "Custom SNEX class for wrapper " << p;

		Struct s(*this, u->nodeTree[PropertyIds::ID].toString(), {}, {});
		addComment(comment, Base::CommentType::FillTo80Light);


		*this << code;

		if (outputFormat == Format::CppDynamicLibrary)
		{
			addIfNotEmptyLine();
			*this << "void initialise(NodeBase* n) {}";

			if (code.contains("Math"))
			{
				addIfNotEmptyLine();
				*this << "hmath Math;";
			}
		}

		s.flushIfNot();

		addEmptyLine();

		*u << s.toExpression();
	}

	return parseMod(u);
}

Node::Ptr ValueTreeBuilder::parseContainer(Node::Ptr u)
{
	if (FactoryIds::isContainer(getNodePath(u->nodeTree)))
	{
		for (auto c : u->nodeTree.getChildWithName(PropertyIds::Nodes))
			pooledTypeDefinitions.add(parseNode(c));

		parseContainerParameters(u);
		parseContainerChildren(u);

		auto needsInitialisation = u->isRootNode();
		
#if 0
		[](Node::Ptr u)
		{
			auto hasParameters = u->nodeTree.getChildWithName(PropertyIds::Parameters).getNumChildren() != 0;

			bool hasNodesWithParameters = false;

			for (auto c : u->nodeTree.getChildWithName(PropertyIds::Nodes))
			{
				hasNodesWithParameters |= c.getChildWithName(PropertyIds::Parameters).getNumChildren() != 0;
			}

			return hasParameters || hasNodesWithParameters;
		};
#endif

		if (needsInitialisation)
		{
			u = parseRootContainer(u);
		}

		auto realPath = u->nodeTree[PropertyIds::FactoryPath].toString().fromFirstOccurrenceOf("container.", false, false);

		if (realPath.startsWith("frame"))
		{
			u = wrapNode(u, NamespacedIdentifier::fromString("wrap::frame"), (int)u->nodeTree[PropertyIds::NumChannels]);
		}
		if (realPath.startsWith("midi"))
		{
			u = wrapNode(u, NamespacedIdentifier::fromString("wrap::event"));
		}
		if (realPath.startsWith("fix"))
		{
			auto bs = realPath.fromFirstOccurrenceOf("fix", false, false).getIntValue();
			u = wrapNode(u, NamespacedIdentifier::fromString("wrap::fix_block"), bs);
		}
		if (realPath.startsWith("oversample"))
		{
			auto os = realPath.fromFirstOccurrenceOf("oversample", false, false).getIntValue();
			u = wrapNode(u, NamespacedIdentifier::fromString("wrap::oversample"), os);
		}

		jassert(!u->isFlushed());

		addNodeComment(u);
		u->flushIfNot();
	}

	return parseMod(u);
}

void ValueTreeBuilder::parseContainerChildren(Node::Ptr container)
{
	Node::List children;

    auto nodeTree = container->nodeTree.getChildWithName(PropertyIds::Nodes);
    
	ValueTreeIterator::forEach(nodeTree, ValueTreeIterator::OnlyChildren, [&](ValueTree& c)
	{
		auto parentPath = getNodePath(ValueTreeIterator::findParentWithType(c, PropertyIds::Node));

		auto useFix = c.getParent().indexOf(c) == 0 || parentPath.getIdentifier() == Identifier("multi");

		if (useFix)
			children.add(parseFixChannel(c));
		else
			children.add(getNode(c, false));

		return false;
	});

	if (children.isEmpty())
	{
		UsingTemplate u(*this, "empty", NamespacedIdentifier::fromString("core::empty"));

		*container << u;
	}
		

	for (auto& c : children)
	{
		*container << *c;
	}
}

void ValueTreeBuilder::parseContainerParameters(Node::Ptr c)
{
	addEmptyLine();

	auto pTree = c->nodeTree.getChildWithName(PropertyIds::Parameters);
	auto numParameters = pTree.getNumChildren();

	Namespace n(*this, c->scopedId.getIdentifier().toString() + "_parameters", !ValueTreeIterator::hasRealParameters(c->nodeTree));

	if(numParameters == 0)
		*c << "parameter::empty";
	else if (numParameters == 1)
	{
		auto up = parseParameter(pTree.getChild(0), Connection::CableType::Parameter);
		n.flushIfNot();
		*c << *up;
	}
	else
	{
		addComment("Parameter list for " + c->scopedId.toString(), Base::CommentType::FillTo80Light);

		PooledParameter::List parameterList;

		for (auto c : pTree)
			parameterList.add(parseParameter(c, Connection::CableType::Parameter));

		String pId;
		pId << c->scopedId.getIdentifier();
		pId << "_plist";

		Node::Ptr l = createNode(pTree, pId, "parameter::list");

		for (auto p : parameterList)
		{
			p->flushIfNot();
			*l << *p;
		}

		l->flushIfNot();
		n.flushIfNot();
		*c << *l;
	}
}



Node::Ptr ValueTreeBuilder::parseRootContainer(Node::Ptr container)
{
	jassert(container->isRootNode());
	RootContainerBuilder rb(*this, container);

	return rb.parse();
}


Node::Ptr ValueTreeBuilder::parseComplexDataNode(Node::Ptr u)
{
	if (ValueTreeIterator::isComplexDataNode(u->nodeTree))
	{
		ComplexDataBuilder c(*this, u);
		return c.parse();
	}

	return u;
}

PooledParameter::Ptr ValueTreeBuilder::parseParameter(const ValueTree& p, Connection::CableType connectionType)
{
	ValueTree cTree;

	auto pId = p[PropertyIds::ID].toString();

	if (connectionType == Connection::CableType::Parameter)
	{
		cTree = p.getChildWithName(PropertyIds::Connections);
	}
	else
	{
		cTree = p.getChildWithName(PropertyIds::ModulationTargets);

		jassert(cTree.isValid());

		pId << "_mod";
	}

	auto numConnections = cTree.getNumChildren();

	if (numConnections == 0)
		return createParameterFromConnection({}, pId, -1);

	else if (numConnections == 1)
	{
		return createParameterFromConnection(getConnection(cTree.getChild(0)), pId, -1);
	}
	else
	{
		auto ch = makeParameter(pId, "chain", {});

		Array<Connection> chainList;

		for (auto c : cTree)
		{
			chainList.add(getConnection(c));
		}

		auto inputRange = RangeHelpers::getDoubleRange(p);

		if(RangeHelpers::isIdentity(inputRange))
			*ch << "ranges::Identity";
		else
		{
			String fWhat;
			fWhat << pId << "_InputRange";
		}

		int cIndex = 0;
		for (auto& c : chainList)
		{
			auto up = createParameterFromConnection(c, pId, cIndex++);
			*ch << *up;
		}

		addIfNotEmptyLine();
		ch->flushIfNot();
		addEmptyLine();

		return addParameterAndReturn(ch);
	}
}

Node::Ptr ValueTreeBuilder::wrapNode(Node::Ptr u, const NamespacedIdentifier& wrapId, int firstIntParam)
{
	// The id is already used by the wrapped node...
	jassert(!u->isFlushed());

	Node::Ptr wn = new Node(*this, u->scopedId.id, wrapId);

	if (u->getLength() > 30)
	{
		StringHelpers::addSuffix(u->scopedId, "_");
		u->flushIfNot();
		addEmptyLine();
	}

	if(firstIntParam != -1)
		*wn << firstIntParam;
	*wn << *u;

	return wn;
}

Connection ValueTreeBuilder::getConnection(const ValueTree& c)
{
	jassert(c.getType() == PropertyIds::Connection || c.getType() == PropertyIds::ModulationTarget);

	String id = getGlueCode(FormatGlueCode::WrappedNamespace);
	
	id << "::" << c[PropertyIds::NodeId].toString();
	id << "_t";

	auto pId = c[PropertyIds::ParameterId].toString();
	auto nId = NamespacedIdentifier::fromString(id);

	Connection rc;

	rc.targetRange = RangeHelpers::getDoubleRange(c);
	
	if (c.getType() == PropertyIds::ModulationTarget)
		rc.cableType = Connection::CableType::Modulation;
	else
		rc.cableType = Connection::CableType::Parameter;

	if (pId == PropertyIds::Bypassed.toString())
		rc.index = scriptnode::bypass::ParameterId;

	rc.expressionCode = c[PropertyIds::Expression].toString();

	ValueTreeIterator::forEach(v, ValueTreeIterator::Forward, [&](ValueTree& t)
	{
		if (t.getType() == PropertyIds::Node)
		{
			if (getNodeId(t) == nId)
			{
				auto pTree = t.getChildWithName(PropertyIds::Parameters);
				auto c = pTree.getChildWithProperty(PropertyIds::ID, pId);

				if(rc.getType() != Connection::ConnectionType::Bypass)
					rc.index = pTree.indexOf(c);

				if (getNode(t, true) == nullptr)
					pooledTypeDefinitions.add(parseNode(t));

				return true;
			}
		}

		return false;
	});

	for (auto n : pooledTypeDefinitions)
	{
		if (n->scopedId == nId)
		{
			rc.n = n;
			return rc;
		}
	}

	return {};
}

Node::Ptr ValueTreeBuilder::parseMod(Node::Ptr u)
{
	auto modTree = u->nodeTree.getChildWithName(PropertyIds::ModulationTargets);

	if (modTree.isValid() && modTree.getNumChildren() > 0)
	{
		addEmptyLine();

		auto mod = parseParameter(u->nodeTree, Connection::CableType::Modulation);

		mod->flushIfNot();

		auto id = getNodeId(u->nodeTree);

		if (!ValueTreeIterator::needsModulationWrapper(u->nodeTree))
		{
			*u << *mod;
		}
		else
		{
			auto inner = u;
			u = createNode(u->nodeTree, id.getIdentifier(), "wrap::mod");
			*u << *mod;
			*u << *inner;
		}

		addNodeComment(u);

		u = parseComplexDataNode(u);

		u->flushIfNot();

		return u;
	}

	addNodeComment(u);

	return parseComplexDataNode(u);
}



Node::Ptr ValueTreeBuilder::getTypeDefinition(const NamespacedIdentifier& id)
{
	for (auto& c : pooledTypeDefinitions)
	{
		if (c->scopedId == id)
			return c;
	}

	return nullptr;
}

NamespacedIdentifier ValueTreeBuilder::getNodePath(const ValueTree& n)
{
	auto s = n[scriptnode::PropertyIds::FactoryPath].toString().replace(".", "::");

	auto fId = NamespacedIdentifier::fromString(s);

	if (FactoryIds::isContainer(fId))
	{
		auto isSplit = fId.id.toString() == "split";
		auto isMulti = fId.id.toString() == "multi";

		if (!isSplit && !isMulti)
		{
			return NamespacedIdentifier::fromString("container::chain");
		}
	}

	return NamespacedIdentifier::fromString(s);
}



NamespacedIdentifier ValueTreeBuilder::getNodeId(const ValueTree& n)
{
	auto typeId = n[scriptnode::PropertyIds::ID].toString();
	typeId << "_t";

	return NamespacedIdentifier(getGlueCode(FormatGlueCode::WrappedNamespace)).getChildId(typeId);
}

NamespacedIdentifier ValueTreeBuilder::getNodeVariable(const ValueTree& n)
{
	auto typeId = n[scriptnode::PropertyIds::ID].toString();
	return getCurrentScope().getChildId(typeId);
}

bool ValueTreeIterator::isBetween(IterationType l, IterationType u, IterationType v)
{
	return v >= l && v <= u;
}

bool ValueTreeIterator::isBackwards(IterationType t)
{
	return t == Backwards || t == ChildrenFirstBackwards || t == OnlyChildrenBackwards;
}

bool ValueTreeIterator::isRecursive(IterationType t)
{
	return !isBetween(OnlyChildren, OnlyChildrenBackwards, t);
}

bool ValueTreeIterator::forEach(ValueTree v, IterationType type, const std::function<bool(ValueTree& v)>& f)
{
	if (isBetween(Forward, Backwards, type))
	{
		if (f(v))
			return true;
	}

	if (isBackwards(type))
	{
		for (int i = v.getNumChildren() - 1; i >= 0; i--)
		{
			if (isRecursive(type))
			{
				if (forEach(v.getChild(i), type, f))
					return true;
			}
			else
			{
                auto c = v.getChild(i);
				if (f(c))
					return true;
			}
		}
	}
	else
	{
		for (auto c : v)
		{
			if (isRecursive(type))
			{
				if (forEach(c, type, f))
					return true;
			}
			else
			{
				if (f(c))
					return true;
			}
		}
	}

	if (isBetween(ChildrenFirst, ChildrenFirstBackwards, type))
	{
		if (f(v))
			return true;
	}

	return false;
}



bool ValueTreeIterator::forEachParent(ValueTree& v, const Func& f)
{
	if (!v.isValid())
		return false;

	if (f(v))
		return true;

    
    auto p = v.getParent();
	return forEachParent(p, f);
}

bool ValueTreeIterator::needsModulationWrapper(ValueTree& v)
{
	if (v.getChildWithName(PropertyIds::ModulationTargets).getNumChildren() == 0)
		return false;

	auto id = v.getProperty(PropertyIds::FactoryPath).toString();

	if (id == "core.pma")
		return false;

	if (id == "core.cable_table")
		return false;

	return true;
}

bool ValueTreeIterator::getNodePath(Array<int>& path, ValueTree& root, const Identifier& id)
{
	if (root[PropertyIds::ID].toString() == id.toString())
	{
		return true;
	}

	auto n = root.getChildWithName(PropertyIds::Nodes);

	for (auto c : n)
	{
		if (getNodePath(path, c, id))
		{
			path.insert(0, getIndexInParent(c));
			return true;
		}
	}

	return false;
}

bool ValueTreeIterator::hasRealParameters(const ValueTree& containerTree)
{
	auto pTree = containerTree.getChildWithName(PropertyIds::Parameters);

	for (auto p : pTree)
	{
		auto c = p.getChildWithName(PropertyIds::Connections);

		if (c.getNumChildren() != 0)
			return true;
	}

	return false;
}

bool ValueTreeIterator::hasNodeProperty(const ValueTree& nodeTree, const Identifier& id)
{
	auto propTree = nodeTree.getChildWithName(PropertyIds::Properties);
	return propTree.getChildWithProperty(PropertyIds::ID, id.toString()).isValid();
}

juce::var ValueTreeIterator::getNodeProperty(const ValueTree& nodeTree, const Identifier& id)
{
	if (!hasNodeProperty(nodeTree, id))
		return {};

	auto propTree = nodeTree.getChildWithName(PropertyIds::Properties);
	return propTree.getChildWithProperty(PropertyIds::ID, id.toString())[PropertyIds::Value];
}

bool ValueTreeIterator::isComplexDataNode(const ValueTree& nodeTree)
{
	jassertfalse;
	return false;
}

bool ValueTreeIterator::isAutomated(const ValueTree& parameterTree)
{
	auto root = parameterTree.getRoot();

	auto pId = Identifier(parameterTree[PropertyIds::ID].toString());
	auto nodeId = Identifier(findParentWithType(parameterTree, PropertyIds::Node)[PropertyIds::ID].toString());

	return forEach(root, Forward, [pId, nodeId](ValueTree& v)
	{
		if (v.getType() == PropertyIds::Connection || v.getType() == PropertyIds::ModulationTarget)
		{
			auto thisP = Identifier(v[PropertyIds::ParameterId].toString());
			auto thisNode = Identifier(v[PropertyIds::NodeId].toString());

			if (thisP == pId && thisNode == nodeId)
				return true;
		}

		return false;
	});
}

String ValueTreeIterator::getSnexCode(const ValueTree& nodeTree)
{
	auto propTree = nodeTree.getChildWithName(PropertyIds::Properties);
	auto c = propTree.getChildWithProperty(PropertyIds::ID, PropertyIds::Code.toString());
	return c[PropertyIds::Value].toString();
}

bool ValueTreeBuilder::Sanitizers::setChannelAmount(ValueTree& v)
{
	int numChannels = v[PropertyIds::NumChannels];

	auto n = v.getChildWithName(PropertyIds::Nodes);

	if (n.isValid())
	{
		for (auto c : n)
		{
			if (numChannels < (int)c[PropertyIds::NumChannels])
			{
				c.setProperty(PropertyIds::NumChannels, numChannels, nullptr);
			}

			setChannelAmount(c);
		}
	}

	return false;
}

snex::cppgen::Node::Ptr ValueTreeBuilder::RootContainerBuilder::parse()
{
	if (root->getLength() > 60)
	{
		root->scopedId = root->scopedId.getParent().getChildId(root->scopedId.getIdentifier().toString() + "_");
		root->flushIfNot();
		parent.addEmptyLine();
	}


	nodeClassId = Identifier(parent.getGlueCode(FormatGlueCode::MainInstanceClass));
	parent.addComment("Root node initialiser class", Base::CommentType::FillTo80);

	Struct s(parent, nodeClassId, { root }, {});

	addMetadata();

	{
		String f;
		f << nodeClassId << "()";
		parent << f;

		{
			StatementBlock sb(parent);

			int index = 0;

			parent.addComment("Node References", Base::CommentType::FillTo80Light);
			
			createStackVariablesForChildNodes();

			addParameterConnections();
			addModulationConnections();
			addSendConnections();

			addDefaultParameters();
		}

		if (hasComplexTypes())
		{

			parent.addEmptyLine();

			parent << "void setExternalData(const ExternalData& b, int index)";

			stackVariables.clear();

			StatementBlock sb(parent);

			parent.addComment("External Data Connections", Base::CommentType::FillTo80Light);

			ValueTreeIterator::forEach(root->nodeTree.getChildWithName(PropertyIds::Nodes), ValueTreeIterator::Forward, [&](ValueTree& childNode)
			{
				if (childNode.getType() == PropertyIds::Node)
				{
					if (ComplexDataBuilder::getType(childNode) != ExternalData::DataType::numDataTypes)
					{
						auto sv = getChildNodeAsStackVariable(childNode);
						

						String def;

						def << sv->toExpression() << ".setExternalData(b, index);";

						parent << def;

						auto type = parent.getNode(childNode, false);
						parent.addComment(type->toExpression(), Base::CommentType::AlignOnSameLine);
					}
				}

				return false;
			});

		}
	}

	

	s.flushIfNot();

	return parent.createNode(root->nodeTree, nodeClassId, {});
}



void ValueTreeBuilder::RootContainerBuilder::addDefaultParameters()
{
	parent.addComment("Default Values", Base::CommentType::FillTo80Light);

	for (auto sv : stackVariables)
	{
		int pIndex = 0;

		auto child = sv->nodeTree;

		auto pTree = child.getChildWithName(PropertyIds::Parameters);

		auto numParameters = getNumParametersToInitialise(child);

		for (auto p : pTree)
		{
			String comment;

			auto np = NamespacedIdentifier(child[PropertyIds::ID].toString());

			if (ValueTreeIterator::isAutomated(p))
			{
				parent << ";";
				comment << "" << np.getChildId(p[PropertyIds::ID].toString()).toString();
				comment << " is automated";
				parent.addComment(comment, Base::CommentType::AlignOnSameLine);
				continue;
			}

			String v;
			v << sv->toExpression() << ".setParameter<" << pTree.indexOf(p) << ">(";
			v << Types::Helpers::getCppValueString(p[PropertyIds::Value], Types::ID::Double);
			v << ");";
			parent << v;

			if(!FactoryIds::isContainer(np))
				np = parent.getNodePath(child);

			comment << np.getChildId(p[PropertyIds::ID].toString()).toString();

			parent.addComment(comment, Base::CommentType::AlignOnSameLine);
		}

		parent.addEmptyLine();
	}

	for (auto p : root->nodeTree.getChildWithName(PropertyIds::Parameters))
	{
		String l;
		l << "this->setParameter<" << ValueTreeIterator::getIndexInParent(p) << ">(";
		l << Helpers::getCppValueString(p[PropertyIds::Value], Types::ID::Double) << ");";
		parent << l;
	}

	if (outputFormat == Format::CppDynamicLibrary && hasComplexTypes())
	{
		parent << "setExternalData({}, -1);";
	}
}

PooledStackVariable::Ptr ValueTreeBuilder::RootContainerBuilder::getChildNodeAsStackVariable(const ValueTree& v)
{
	Array<int> path;

	auto id = v[PropertyIds::ID].toString();

	ValueTreeIterator::getNodePath(path, root->nodeTree, id);

	String tid = id;

	PooledStackVariable::Ptr e = new PooledStackVariable(parent, v);
	
	int index = 0;

	*e << "this->";

	for (auto i : path)
	{
		*e << "get<" << i << ">()";

		if (++index != path.size())
			*e << JitTokens::dot;
	}

	if(path.size() > 4)
		e->addBreaksBeforeDots(path.size() / 2);

	if (auto existing = stackVariables.getExisting(*e))
		return existing;

	return e;
}

snex::cppgen::PooledStackVariable::Ptr ValueTreeBuilder::RootContainerBuilder::getChildNodeAsStackVariable(const String& nodeId)
{
	for (auto n : stackVariables)
	{
		if (n->nodeTree[PropertyIds::ID].toString() == nodeId)
			return n;
	}

	return nullptr;
}




void ValueTreeBuilder::RootContainerBuilder::addMetadata()
{
	Struct m(parent, "metadata", {}, {});

	Macro(parent, "SNEX_METADATA_ID", { root->nodeTree[PropertyIds::ID].toString() });
	Macro(parent, "SNEX_METADATA_NUM_CHANNELS", { root->nodeTree[PropertyIds::NumChannels].toString() });

	auto pCopy = root->nodeTree.getChildWithName(PropertyIds::Parameters).createCopy();

	scriptnode::parameter::encoder encoder(pCopy);

	int ssi = sizeof(NormalisableRange<double>);

	cppgen::EncodedParameterMacro(parent, encoder);

#if 0
	if (outputFormat == Format::CppDynamicLibrary)
	{
		String s;
		zstd::ZDefaultCompressor comp;
		MemoryBlock mb;
		comp.compress(root->nodeTree, mb);
		s = mb.toBase64Encoding();
		Macro(parent, "SNEX_METADATA_SNIPPET", { s });
	}
#endif

	m.flushIfNot();
	parent.addEmptyLine();
}

void ValueTreeBuilder::RootContainerBuilder::addParameterConnections()
{
	auto index = 0;

	auto pList = getContainersWithParameter();

	if (!pList.isEmpty())
	{
		parent.addComment("Parameter Connections", Base::CommentType::FillTo80Light);

		for (auto containerWithParameter : pList)
		{
			for (auto p : containerWithParameter->nodeTree.getChildWithName(PropertyIds::Parameters))
			{
				String def;

				PooledStackVariable::Ptr c = getChildNodeAsStackVariable(containerWithParameter->nodeTree);
				
				auto pId = p[PropertyIds::ID].toString();
				pId << "_p";

				StackVariable pv(parent, pId, TypeInfo(Types::ID::Dynamic, false, true));

				if (containerWithParameter->isRootNode())
				{
					pv << "this->";
				}
				else
				{
					pv << *c;
					pv << ".";
				}

				pv << "getParameter<" << ValueTreeIterator::getIndexInParent(p) << ">()";

				auto cTree = p.getChildWithName(PropertyIds::Connections);

				if (cTree.getNumChildren() > 1)
					pv.flushIfNot();

				for (auto c_ : cTree)
				{
					auto targetId = Identifier(c_[PropertyIds::NodeId].toString());

					String def;
					
					auto cIndex = ValueTreeIterator::getIndexInParent(c_);

					def << pv.toExpression() << ".connect<" << cIndex << ">(";

					bool found = false;

					for (auto nv : stackVariables)
					{
						if (nv->scopedId.getIdentifier() == targetId)
						{
							def << nv->toExpression();

							found = true;
							break;
						}
					}

					if (!found)
					{
						jassertfalse;

						auto e = getChildNodeAsStackVariable(c_);
						def << e->toExpression();
					}

					def << ");";

					parent << def;

					String comment;

					comment << p[PropertyIds::ID].toString() << " -> " << targetId << "::" << c_[PropertyIds::ParameterId].toString();

					parent.addComment(comment, Base::CommentType::AlignOnSameLine);
				}

				if (!ValueTreeIterator::isLast(p))
					parent.addEmptyLine();
			}

			
		}
	}

	parent.addIfNotEmptyLine();
}

void ValueTreeBuilder::RootContainerBuilder::addModulationConnections()
{
	auto modList = getModulationNodes();

	if (modList.size() > 0)
	{
		parent.addEmptyLine();
		parent.addComment("Modulation Connections", Base::CommentType::FillTo80Light);

		for (auto m : modList)
		{
			Array<int> path;

			auto sourceId = m->nodeTree[PropertyIds::ID].toString();
			auto ms = getChildNodeAsStackVariable(m->nodeTree);
			auto mtargetTree = m->nodeTree.getChildWithName(PropertyIds::ModulationTargets);

			auto needsModWrapper = ValueTreeIterator::needsModulationWrapper(m->nodeTree);

			for (auto t : mtargetTree)
			{
				auto cn = parent.getConnection(t);

				if (!cn.n->nodeTree.isValid())
				{
					Error e;
					e.errorMessage = "No ValueTree for node " + cn.n->toString();
					throw e;
				}

				String def;

				def << ms->toExpression();
				
				if (!needsModWrapper)
					def << ".getWrappedObject()";

				def << ".getParameter().connect<" << ValueTreeIterator::getIndexInParent(t) << ">(";
				auto targetId = cn.n->nodeTree[PropertyIds::ID].toString();
				auto mt = getChildNodeAsStackVariable(cn.n->nodeTree);

				if (mt->getLength() > 20)
				{
					mt->flushIfNot();
					parent.addComment(targetId, Base::CommentType::AlignOnSameLine);
				}

				def << mt->toExpression();
				def << ");";

				parent << def;

				String comment;
				comment << sourceId << " -> " << targetId << "::" << t[PropertyIds::ParameterId].toString();

				parent.addComment(comment, CommentType::AlignOnSameLine);
			}
		}
	}

	parent.addEmptyLine();
}

void ValueTreeBuilder::RootContainerBuilder::addSendConnections()
{
	auto sendList = getSendNodes();

	if (sendList.isEmpty())
		return;

	parent.addComment("Send Connections", Base::CommentType::FillTo80Light);

	for (auto s : sendList)
	{
		auto propTree = s->nodeTree.getChildWithName(PropertyIds::Properties);
		auto c = propTree.getChildWithProperty(PropertyIds::ID, PropertyIds::Connection.toString());
		auto ids = c[PropertyIds::Value].toString();

		auto source = getChildNodeAsStackVariable(s->nodeTree);

		for (auto& id : StringArray::fromTokens(ids, ";", ""))
		{
			auto target = getChildNodeAsStackVariable(id);

			if (target == nullptr)
			{
				Error e;
				e.v = s->nodeTree;
				e.errorMessage << id << " not found";
				throw e;
			}

			String def;

			def << source->toExpression() << ".connect(" << target->toExpression() << ");";
			parent << def;
		}

	}

	parent.addEmptyLine();
}

void ValueTreeBuilder::RootContainerBuilder::createStackVariablesForChildNodes()
{
	ValueTreeIterator::forEach(root->nodeTree.getChildWithName(PropertyIds::Nodes), ValueTreeIterator::Forward, [&](ValueTree& childNode)
	{
		if (childNode.getType() == PropertyIds::Node)
		{
			auto sv = getChildNodeAsStackVariable(childNode);
			
			sv->flushIfNot();

			auto type = parent.getNode(childNode, false);
			parent.addComment(type->toExpression(), Base::CommentType::AlignOnSameLine);
			
			stackVariables.add(sv);
		}

		return false;
	});

	parent.addIfNotEmptyLine();
}

int ValueTreeBuilder::RootContainerBuilder::getNumParametersToInitialise(ValueTree& child)
{
	int numParameters = 0;

	for (auto p : child.getChildWithName(PropertyIds::Parameters))
	{
		if (!ValueTreeIterator::isAutomated(p))
			numParameters++;
	}

	return numParameters;
}

Node::List ValueTreeBuilder::RootContainerBuilder::getSendNodes()
{
	Node::List l;

	for (auto n : parent.pooledTypeDefinitions)
	{
		if (getNodePath(n->nodeTree).toString() == "routing::send")
		{
			l.add(n);
		}
	}

	return l;
}

snex::cppgen::Node::List ValueTreeBuilder::RootContainerBuilder::getModulationNodes()
{
	Node::List l;

	for (auto u : parent.pooledTypeDefinitions)
	{
		if (u->nodeTree.getChildWithName(PropertyIds::ModulationTargets).getNumChildren() > 0)
			l.addIfNotAlreadyThere(u);
	}

	return l;
}

snex::cppgen::Node::List ValueTreeBuilder::RootContainerBuilder::getContainersWithParameter()
{
	Node::List l;

	for (auto u : parent.pooledTypeDefinitions)
	{
		if (FactoryIds::isContainer(parent.getNodePath(u->nodeTree)))
		{
			if (ValueTreeIterator::hasRealParameters(u->nodeTree))
				l.add(u);
		}
	}

	if (ValueTreeIterator::hasRealParameters(root->nodeTree))
		l.addIfNotAlreadyThere(root);

	return l;
}

bool ValueTreeBuilder::RootContainerBuilder::hasComplexTypes() const
{
	return ValueTreeIterator::forEach(root->nodeTree, ValueTreeIterator::Forward, [](ValueTree& v)
	{
		if (v.getType() == PropertyIds::Node && ComplexDataBuilder::getType(v) != ExternalData::DataType::numDataTypes)
			return true;

		return false;
	});
}

PooledStackVariable::PooledStackVariable(Base& p, const ValueTree& nodeTree_) :
	StackVariable(p, nodeTree_[PropertyIds::ID].toString(), TypeInfo(Types::ID::Dynamic, false, true)),
	nodeTree(nodeTree_)
{

}

ValueTreeBuilder::ComplexDataBuilder::ComplexDataBuilder(ValueTreeBuilder& parent_, Node::Ptr nodeToWrap):
	n(nodeToWrap),
	parent(parent_)
{

}

Node::Ptr ValueTreeBuilder::ComplexDataBuilder::parse()
{
#if 0
	auto dataIndex = ValueTreeIterator::getNodeProperty(n->nodeTree, PropertyIds::DataIndex);

	// The id is already used by the wrapped node...
	jassert(!n->isFlushed());

	Node::Ptr wn = new Node(parent, n->scopedId.id, NamespacedIdentifier("wrap::data"));

	wn->nodeTree = n->nodeTree;

	*wn << *n;
	*wn << *parseDataClass();

	

	wn->flushIfNot();

	return wn;
#endif
	jassertfalse;
	return nullptr;
}

Node::Ptr ValueTreeBuilder::ComplexDataBuilder::parseDataClass()
{
#if 0
	auto dataIndex = (int)ValueTreeIterator::getNodeProperty(n->nodeTree, PropertyIds::DataIndex);

	NamespacedIdentifier d("data");

	String expr;

	if (dataIndex == -1)
	{
		auto sId = n->scopedId;
		StringHelpers::addSuffix(sId, "_data");

		Struct s(parent, sId.getIdentifier(), {}, {});

		FloatArray(parent, "data", getEmbeddedData());
		
		
		s.flushIfNot();

		parent.addEmptyLine();

		d = d.getChildId("embedded");

		expr = s.toExpression();
		
	}
	else
	{
		d = d.getChildId("external");

		expr = String(dataIndex);
	}

	d = d.getChildId(ExternalData::getDataTypeName(getType(n->nodeTree)).toLowerCase());

	Node::Ptr dc = new Node(parent, {}, d);

	*dc << expr;

	return dc;
#endif

	jassertfalse;
	return nullptr;
}



snex::ExternalData::DataType ValueTreeBuilder::ComplexDataBuilder::getType(const ValueTree& v)
{
#if 0
	if (v.hasProperty(PropertyIds::NumTables))
		return ExternalData::DataType::Table;

	if (v.hasProperty(PropertyIds::NumAudioFiles))
		return ExternalData::DataType::AudioFile;

	if (v.hasProperty(PropertyIds::NumSliderPacks))
		return ExternalData::DataType::SliderPack;

	return ExternalData::DataType::numDataTypes;
#endif

	// This should not work because there can be multiple data types...
	jassertfalse;
	return ExternalData::DataType::numDataTypes;
}

juce::Array<float> ValueTreeBuilder::ComplexDataBuilder::getEmbeddedData() const
{
	jassertfalse;


	auto t = getType(n->nodeTree);

	Array<float> data;

#if 0
	if (t == ExternalData::DataType::Table)
	{
		SampleLookupTable table;
		auto d = ValueTreeIterator::getNodeProperty(n->nodeTree, PropertyIds::EmbeddedData);
		
		table.restoreData(d);

		data.ensureStorageAllocated(SAMPLE_LOOKUP_TABLE_SIZE);

		auto ptr = table.getReadPointer();

		for (int i = 0; i < SAMPLE_LOOKUP_TABLE_SIZE; i++)
			data.add(ptr[i]);
	}
#endif

	return data;

}

}
}