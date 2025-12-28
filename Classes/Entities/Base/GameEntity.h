#ifndef __GAME_ENTITY_H__
#define __GAME_ENTITY_H__

#include "cocos2d.h"
#include "Core/Constants.h"
#include "Core/GameMacros.h"

USING_NS_CC;

// 游戏实体基类
class GameEntity : public Node {
public:
    GameEntity();
    virtual ~GameEntity();
    
    virtual bool init() override;
    virtual void update(float dt) override;
    
    // 精灵管理
    // 绑定显示精灵
    virtual void bindSprite(Sprite* sprite, int zOrder = Constants::ZOrder::ENTITY);
    
    // 获取精灵对象
    Sprite* getSprite() const { return _sprite; }
    
    // 生命值管理
    // 获取当前生命值
    int getHP() const { return _hp; }
    
    // 设置生命值
    void setHP(int hp) { _hp = hp; }
    
    // 获取最大生命值
    int getMaxHP() const { return _maxHP; }
    
    // 设置最大生命值
    void setMaxHP(int maxHP) { _maxHP = maxHP; }
    
    // 扣除生命值（向后兼容，旧接口）
    virtual void takeDamage(int damage);

    // 扣血并返回"实际对该实体造成的 HP 减少值"
    // 用于需要显示"实际生效伤害"的调用点（例如浮动文字）
    // 默认实现包含原有的扣血/闪烁/死亡逻辑并返回实际减少量
    virtual int takeDamageReported(int damage);
    
    // 治疗
    virtual void heal(int heal);
    
    // 检查是否死亡
    bool isDead() const { return _hp <= 0; }
    
    // 碰撞检测
    // 获取碰撞边界框
    Rect getBoundingBox() const;
    
    // 检测与另一个实体的碰撞
    bool checkCollision(GameEntity* other) const;
    
    // 死亡处理
    // 死亡处理(纯虚函数，子类必须实现)
    virtual void die() = 0;
    
    // 显示死亡效果
    virtual void showDeathEffect();
    
protected:
    Sprite* _sprite;              // 显示精灵
    
    int _hp;                      // 当前生命值
    int _maxHP;                   // 最大生命值
    
    bool _isAlive;                // 是否存活

    // 受击无敌计时器
    float _hitInvulTimer;
    static constexpr float HIT_INVUL_DURATION = 0.1f;
};

#endif // __GAME_ENTITY_H__
