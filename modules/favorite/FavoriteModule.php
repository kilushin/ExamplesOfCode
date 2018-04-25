<?php

/**
 * Class FavoriteModule
 */
class FavoriteModule extends \cms\components\WebModule
{
    /**
     *
     */
    const VERSION = '1.2';

    /**
     *
     */
    public function init()
    {
        parent::init();

        $this->setImport(array(
            'favorite.components.*',
        ));
    }

    /**
     * @return bool
     */
    public function getIsShowInAdminMenu()
    {
        return false;
    }

    /**
     * @return array
     */
    public function getDependencies()
    {
        return ['store'];
    }

    /**
     * @return bool
     */
    public function getAdminPageLink()
    {
        return false;
    }

    /**
     * @return string
     */
    public function getName()
    {
        return Yii::t('FavoriteModule.favorite', 'Favorite');
    }

    /**
     * @return string
     */
    public function getCategory()
    {
        return Yii::t('FavoriteModule.favorite', 'Store');
    }

    /**
     * @return string
     */
    public function getDescription()
    {
        return Yii::t('FavoriteModule.favorite', 'Favorite products module');
    }

    /**
     * @return string
     */
    public function getVersion()
    {
        return self::VERSION;
    }

    /**
     * @return string
     */
    public function getAuthor()
    {
        return Yii::t('FavoriteModule.favorite', 'k.ilushin');
    }

    /**
     * @return string
     */
    public function getAuthorEmail()
    {
        return Yii::t('FavoriteModule.favorite', 'k.ilushin@gmail.com');
    }
    /**
     * @return string
     */
    public function getIcon()
    {
        return 'fa fa-fw fa-heart';
    }
}
