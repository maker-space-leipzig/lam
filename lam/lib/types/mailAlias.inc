<?php
/*

  This code is part of LDAP Account Manager (http://www.ldap-account-manager.org/)
  Copyright (C) 2005 - 2024  Roland Gruber

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/**
 * The account type for mail aliases.
 *
 * @package types
 * @author Roland Gruber
 */

/**
 * The account type for mail aliases.
 *
 * @package types
 */
class mailAlias extends baseType {

	/**
	 * Constructs a new alias type object.
	 *
	 * @param ConfiguredType $type configuration
	 */
	public function __construct($type) {
		parent::__construct($type);
		$this->LABEL_CREATE_ANOTHER_ACCOUNT = _('Create another alias');
		$this->LABEL_BACK_TO_ACCOUNT_LIST = _('Back to alias list');
	}

	/**
	 * Returns the alias name of this account type.
	 *
	 * @return string alias name
	 */
	function getAlias() {
		return _("Mail aliases");
	}

	/**
	 * Returns the description of this account type.
	 *
	 * @return string description
	 */
	function getDescription() {
		return _("Mailing aliases (e.g. NIS mail aliases)");
	}

	/**
	 * {@inheritDoc}
	 */
	public function getIcon(): string {
		return 'mail.svg';
	}

	/**
	 * Returns the class name for the list object.
	 *
	 * @return string class name
	 */
	function getListClassName() {
		return "lamMailAliasList";
	}

	/**
	 * Returns the default attribute list for this account type.
	 *
	 * @return string attribute list
	 */
	function getDefaultListAttributes() {
		return "#cn;#rfc822MailMember";
	}

	/**
	 * Returns the the title text for the title bar on the new/edit page.
	 *
	 * @param accountContainer $container account container
	 * @return String title text
	 */
	public function getTitleBarTitle($container) {
		// get attributes
		$attributes = null;
		if ($container->getAccountModule('nisMailAlias') != null) {
			$attributes = $container->getAccountModule('nisMailAlias')->getAttributes();
		}
		// check if a common name is set
		if (isset($attributes['cn'][0])) {
			return htmlspecialchars($attributes['cn'][0]);
		}
		// new entry
		if ($container->isNewAccount) {
			return _("New alias");
		}
		// fall back to default
		return parent::getTitleBarTitle($container);
	}

}


/**
 * Generates the list view.
 *
 * @package lists
 * @author Roland Gruber
 *
 */
class lamMailAliasList extends lamList {

	/**
	 * Constructor
	 *
	 * @param string $type account type
	 * @return lamList list object
	 */
	function __construct($type) {
		parent::__construct($type);
		$this->labels = [
			'nav' => _("Alias count: %s"),
			'error_noneFound' => _("No aliases found!"),
			'newEntry' => _("New alias"),
			'deleteEntry' => _("Delete selected aliases")
		];
	}

}
