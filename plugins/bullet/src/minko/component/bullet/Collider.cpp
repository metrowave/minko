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

#include "Collider.hpp"

#include <minko/math/Matrix4x4.hpp>
#include <minko/scene/Node.hpp>
#include <minko/scene/NodeSet.hpp>
#include <minko/component/Transform.hpp>
#include <minko/component/bullet/AbstractPhysicsShape.hpp>
#include <minko/component/bullet/ColliderData.hpp>
#include <minko/component/bullet/PhysicsWorld.hpp>

using namespace minko;
using namespace minko::math;
using namespace minko::scene;
using namespace minko::component;

bullet::Collider::Collider(ColliderData::Ptr data):
	AbstractComponent(),
	_colliderData(data),
	_physicsWorld(nullptr),
	_targetTransform(nullptr),
	_targetAddedSlot(nullptr),
	_targetRemovedSlot(nullptr),
	_addedSlot(nullptr),
	_removedSlot(nullptr),
	_graphicsTransformChangedSlot(nullptr)
{
	if (data == nullptr)
		throw std::invalid_argument("data");
}

void
bullet::Collider::initialize()
{
	_targetAddedSlot	= targetAdded()->connect(std::bind(
		&bullet::Collider::targetAddedHandler,
		shared_from_this(),
		std::placeholders::_1,
		std::placeholders::_2
		));

	_targetRemovedSlot	= targetRemoved()->connect(std::bind(
		&bullet::Collider::targetRemovedHandler,
		shared_from_this(),
		std::placeholders::_1,
		std::placeholders::_2
		));

	_graphicsTransformChangedSlot	= _colliderData->graphicsWorldTransformChanged()->connect(std::bind(
		&bullet::Collider::graphicsWorldTransformChangedHandler,
		shared_from_this(),
		std::placeholders::_1,
		std::placeholders::_2
		));
}

void
bullet::Collider::targetAddedHandler(
	AbstractComponent::Ptr controller, 
	Node::Ptr target)
{
	if (targets().size() > 1)
		throw std::logic_error("Collider cannot have more than one target.");

	_addedSlot		= targets().front()->added()->connect(std::bind(
		&bullet::Collider::addedHandler,
		shared_from_this(),
		std::placeholders::_1,
		std::placeholders::_2,
		std::placeholders::_3
		));

	_removedSlot	= targets().front()->removed()->connect(std::bind(
		&bullet::Collider::removedHandler,
		shared_from_this(),
		std::placeholders::_1,
		std::placeholders::_2,
		std::placeholders::_3
		));

	// initialize from node if possible (mostly for adding a controller to the camera)
	initializeFromNode(target);
}

void
bullet::Collider::targetRemovedHandler(
	AbstractComponent::Ptr controller, 
	Node::Ptr target)
{
	_addedSlot						= nullptr;
	_removedSlot					= nullptr;
}

void 
bullet::Collider::addedHandler(
	Node::Ptr node, 
	Node::Ptr target, 
	Node::Ptr parent)
{
	initializeFromNode(node);
}

void
bullet::Collider::removedHandler(
	Node::Ptr node, 
	Node::Ptr target, 
	Node::Ptr parent)
{
	if (_physicsWorld != nullptr)
		_physicsWorld->removeChild(_colliderData);

	_physicsWorld		= nullptr;
	_targetTransform	= nullptr;
}

void
bullet::Collider::initializeFromNode(Node::Ptr node)
{
	if (_targetTransform != nullptr && _physicsWorld != nullptr)
		return;

	if (!node->hasComponent<Transform>())
		node->addComponent(Transform::create());
	
	_targetTransform = node->component<Transform>();

	auto nodeSet = NodeSet::create(node)
		->ancestors(true)
		->where([](Node::Ptr node)
		{
			return node->hasComponent<bullet::PhysicsWorld>();
		});

	if (nodeSet->nodes().size() != 1)
	{
#ifdef DEBUG_PHYSICS
		std::cout << "[" << node->name() << "]\tcollider CANNOT be added (# PhysicsWorld = " << nodeSet->nodes().size() << ")." << std::endl;
#endif // DEBUG_PHYSICS

		return;
	}

	_colliderData->name(node->name());

	_physicsWorld	= nodeSet->nodes().front()->component<bullet::PhysicsWorld>();
	_physicsWorld->addChild(_colliderData);

	synchronizePhysicsWithGraphics();
}

void
bullet::Collider::synchronizePhysicsWithGraphics()
{
	if (_physicsWorld == nullptr || _targetTransform == nullptr)
		return;

	auto graphicsTransform = _targetTransform->modelToWorldMatrix(true);

	// remove the influence of scaling and shear
	auto graphicsNoScaleTransform	= Matrix4x4::create();
	auto correction					= Matrix4x4::create();
	PhysicsWorld::removeScalingShear(
		graphicsTransform, 
		graphicsNoScaleTransform, 
		correction
	);

	// record the lost scaling and shear of the graphics transform
	_colliderData->correction(correction);

#ifdef DEBUG_PHYSICS
	std::cout << "[" << _colliderData->name() << "]\tsynchro graphics->physics" << std::endl;
	PhysicsWorld::print(std::cout << "- correction =\n", correction) << std::endl;
	PhysicsWorld::print(std::cout << "- scalefree(graphics) =\n", graphicsNoScaleTransform) << std::endl;
#endif // DEBUG_PHYSICS

	_physicsWorld->synchronizePhysicsWithGraphics(_colliderData, graphicsNoScaleTransform);
}

void
bullet::Collider::graphicsWorldTransformChangedHandler(ColliderData::Ptr collider, 
													   Matrix4x4::Ptr graphicsTransform)
{
	if (_targetTransform == nullptr)
		return;

	// get the world-to-parent matrix in order to update the target's Transform
	auto worldToParent	= Matrix4x4::create()
		->copyFrom(_targetTransform->modelToWorldMatrix(true))
		->invert()
		->append(_targetTransform->transform());

	_targetTransform->transform()
		->copyFrom(graphicsTransform)
		->append(worldToParent);
}

void
bullet::Collider::prependLocalTranslation(Vector3::Ptr localTranslation)
{
	if (_physicsWorld != nullptr)
		_physicsWorld->prependLocalTranslation(_colliderData, localTranslation);
}

void
bullet::Collider::prependRotationY(float radians)
{
	if (_physicsWorld != nullptr)
		_physicsWorld->prependRotationY(_colliderData, radians);
}

void
bullet::Collider::applyRelativeImpulse(Vector3::Ptr localImpulse)
{
	if (_physicsWorld != nullptr)
		_physicsWorld->applyRelativeImpulse(_colliderData, localImpulse);
}