/*
Copyright (c) 2013 Aerys

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "minko/Common.hpp"

#include "minko/scene/Node.hpp"
#include "minko/component/BoundingBox.hpp"

#include "minko/LuaWrapper.hpp"

namespace minko
{
	namespace scene
	{
		class LuaNode
		{
		public:
			static
			void
			bind(LuaGlue& state)
			{
				state.Class<std::vector<Node::Ptr>>("std__vector_scene__Node__Ptr_")
					.method("get",		&LuaNode::getWrapper)
					.property("size", 	&std::vector<Node::Ptr>::size);

				state.Class<Node>("Node")
		            //.method("getName",          static_cast<const std::string& (Node::*)(void)>(&Node::name))
		            //.method("setName",          static_cast<void (Node::*)(const std::string&)>(&Node::name))
		            //.prop("name", &Node::name, &Node::name)
		            .method("create",           static_cast<Node::Ptr (*)(void)>(&Node::create))
		            .method("addChild",         &Node::addChild)
		            .method("removeChild",      &Node::removeChild)
		            .method("contains",         &Node::contains)
		            .method("addComponent",     &Node::addComponent)
		            .method("removeComponent",  &Node::removeComponent)
		            .method("getChildren",		&LuaNode::childrenWrapper)
		            .method("getBoundingBox",	&LuaNode::getBoundingBoxWrapper)
		            .method("getTransform",		&LuaNode::getTransformWrapper)
		            .property("data",           &Node::data)
		            .property("root",           &Node::root);
		            //.property("name",			&Node::name, &Node::name);
			}

			static
			Node::Ptr
			getWrapper(std::vector<Node::Ptr>* v, uint index)
			{
				return (*v)[index - 1];
			}

			static
			std::vector<Node::Ptr>*
			childrenWrapper(Node::Ptr node)
			{
				return const_cast<std::vector<Node::Ptr>*>(&(node->children()));
			}

			static
			std::shared_ptr<component::BoundingBox>
			getBoundingBoxWrapper(Node::Ptr node)
			{
				return node->component<component::BoundingBox>();
			}

			static
			std::shared_ptr<component::Transform>
			getTransformWrapper(Node::Ptr node)
			{
				return node->component<component::Transform>();
			}
		};
	}
}