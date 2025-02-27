<?php
/*

  This code is part of LDAP Account Manager (http://www.ldap-account-manager.org/)
  Copyright (C) 2011 - 2024  Roland Gruber

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
 * Manages FreeRadius accounts.
 *
 * @package modules
 * @author Roland Gruber
 */

use LAM\TYPES\ConfiguredType;

/**
 * Manages FreeRadius accounts.
 *
 * @package modules
 */
class freeRadius extends baseModule {

	/**
	 * These attributes will be ignored by default if a new account is copied from an existing one.
	 */
	private const ATTRIBUTES_TO_IGNORE_ON_COPY = ['radiusFramedIPAddress'];

	/** list of possible months */
	private static $monthList = ['01' => 'Jan', '02' => 'Feb', '03' => 'Mar', '04' => 'Apr', '05' => 'May',
		'06' => 'Jun', '07' => 'Jul', '08' => 'Aug', '09' => 'Sep', '10' => 'Oct', '11' => 'Nov', '12' => 'Dec'
	];

	/** cache for profile DNs */
	private $profileCache;


	/**
	 * Creates a new freeRadius object.
	 *
	 * @param string $scope account type (user, group, host)
	 */
	function __construct($scope) {
		parent::__construct($scope);
		$this->autoAddObjectClasses = false;
	}

	/**
	 * Returns true if this module can manage accounts of the current type, otherwise false.
	 *
	 * @return boolean true if module fits
	 */
	public function can_manage() {
		return in_array($this->get_scope(), ['user']);
	}

	/**
	 * Returns meta data that is interpreted by parent class
	 *
	 * @return array array with meta data
	 *
	 * @see baseModule::get_metaData()
	 */
	function get_metaData() {
		$return = [];
		// icon
		$return['icon'] = 'freeRadius.png';
		// alias name
		$return["alias"] = _("FreeRadius");
		// module dependencies
		$return['dependencies'] = ['depends' => [['posixAccount', 'inetOrgPerson']], 'conflicts' => []];
		// managed object classes
		$return['objectClasses'] = ['radiusprofile'];
		// managed attributes
		$return['attributes'] = ['radiusFramedIPAddress', 'radiusFramedIPNetmask', 'radiusRealm', 'radiusGroupName',
			'radiusExpiration', 'radiusIdleTimeout', 'dialupAccess', 'radiusProfileDn'];
		// help Entries
		$return['help'] = [
			'radiusFramedIPAddress' => [
				"Headline" => _("IP address"), 'attr' => 'radiusFramedIPAddress',
				"Text" => _("This is the IP address for the user (e.g. 123.123.123.123).")
			],
			'radiusFramedIPNetmask' => [
				"Headline" => _("Net mask"), 'attr' => 'radiusFramedIPNetmask',
				"Text" => _("The net mask for the IP address.")
			],
			'radiusRealm' => [
				"Headline" => _("Realm"), 'attr' => 'radiusRealm',
				"Text" => _("The Radius realm of this account.")
			],
			'radiusGroupName' => [
				"Headline" => _("Group names"), 'attr' => 'radiusGroupName',
				"Text" => _("The group names for this account.")
			],
			'radiusGroupNameList' => [
				"Headline" => _("Group names"), 'attr' => 'radiusGroupName',
				"Text" => _("The group names for this account.") . ' ' . _("Multiple values are separated by semicolon.")
			],
			'radiusExpiration' => [
				"Headline" => _("Expiration date"), 'attr' => 'radiusExpiration',
				"Text" => _("The account will be locked after this date.")
			],
			'radiusIdleTimeout' => [
				"Headline" => _("Idle timeout"), 'attr' => 'radiusIdleTimeout',
				"Text" => _("Specifies the maximum number of seconds that a connection can be idle before the session is terminated.")
			],
			'dialupAccess' => [
				"Headline" => _("Enabled"), 'attr' => 'dialupAccess',
				"Text" => _("Specifies if the user may authenticate with FreeRadius.")
			],
			'profileDN' => [
				"Headline" => _("Profile DN"), 'attr' => 'radiusProfileDn',
				"Text" => _('DN where Radius profile templates are stored.')
			],
			'radiusProfileDn' => [
				"Headline" => _("Profile"), 'attr' => 'radiusProfileDn',
				"Text" => _('Radius profile for this user.')
			],
			'hiddenOptions' => [
				"Headline" => _("Hidden options"),
				"Text" => _("The selected options will not be managed inside LAM. You can use this to reduce the number of displayed input fields.")
			],
			'autoAdd' => [
				"Headline" => _("Automatically add this extension"),
				"Text" => _("This will enable the extension automatically if this profile is loaded.")
			],
		];
		// profile settings
		$profileElements = [];
		// auto add extension
		$profileElements[] = new htmlResponsiveInputCheckbox('freeRadius_addExt', false, _('Automatically add this extension'), 'autoAdd');
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusFramedIPNetmask')) {
			$profileElements[] = new htmlResponsiveInputField(_('Net mask'), 'freeRadius_radiusFramedIPNetmask', null, 'radiusFramedIPNetmask');
			$return['profile_checks']['freeRadius_radiusFramedIPNetmask'] = [
				'type' => 'ext_preg',
				'regex' => 'ip',
				'error_message' => $this->messages['radiusFramedIPNetmask'][0]];
			$return['profile_mappings']['freeRadius_radiusFramedIPNetmask'] = 'radiusFramedIPNetmask';
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusRealm')) {
			$profileElements[] = new htmlResponsiveInputField(_('Realm'), 'freeRadius_radiusRealm', null, 'radiusRealm');
			$return['profile_checks']['freeRadius_radiusRealm'] = [
				'type' => 'ext_preg',
				'regex' => 'DNSname',
				'error_message' => $this->messages['radiusRealm'][0]];
			$return['profile_mappings']['freeRadius_radiusRealm'] = 'radiusRealm';
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusGroupName')) {
			$profileElements[] = new htmlResponsiveInputField(_('Group names'), 'freeRadius_radiusGroupName', null, 'radiusGroupNameList');
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusIdleTimeout')) {
			$profileElements[] = new htmlResponsiveInputField(_('Idle timeout'), 'freeRadius_radiusIdleTimeout', null, 'radiusIdleTimeout');
			$return['profile_checks']['freeRadius_radiusIdleTimeout'] = [
				'type' => 'ext_preg',
				'regex' => 'digit',
				'error_message' => $this->messages['radiusIdleTimeout'][0]];
			$return['profile_mappings']['freeRadius_radiusIdleTimeout'] = 'radiusIdleTimeout';
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideDialupAccess')) {
			$enabledOptions = ['-' => '', _('Yes') => 'true', _('No') => 'false'];
			$dialupAccessSelect = new htmlResponsiveSelect('freeRadius_dialupAccess', $enabledOptions, ['true'], _('Enabled'), 'dialupAccess');
			$dialupAccessSelect->setHasDescriptiveElements(true);
			$profileElements[] = $dialupAccessSelect;
			$return['profile_mappings']['freeRadius_dialupAccess'] = 'dialupAccess';
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusProfileDn') && isLoggedIn()) {
			$profileOptions = ['-' => ''];
			foreach ($this->getProfiles() as $dn) {
				$profileOptions[getAbstractDN($dn)] = $dn;
			}
			$profileSelect = new htmlResponsiveSelect('freeRadius_radiusProfileDn', $profileOptions, [''], _('Profile'), 'radiusProfileDn');
			$profileSelect->setHasDescriptiveElements(true);
			$profileElements[] = $profileSelect;
			$return['profile_mappings']['freeRadius_radiusProfileDn'] = 'radiusProfileDn';
		}
		$profileContainer = new htmlResponsiveRow();
		for ($i = 0; $i < sizeof($profileElements); $i++) {
			$profileContainer->add($profileElements[$i], 12);
		}
		$return['profile_options'] = $profileContainer;
		// upload fields
		$return['upload_columns'] = [];
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusRealm')) {
			$return['upload_columns'][] = [
				'name' => 'freeRadius_radiusRealm',
				'description' => _('Realm'),
				'help' => 'radiusRealm',
				'example' => _('company.com')
			];
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusGroupName')) {
			$return['upload_columns'][] = [
				'name' => 'freeRadius_radiusGroupName',
				'description' => _('Group names'),
				'help' => 'radiusGroupNameList',
				'example' => _('group01;group02')
			];
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusFramedIPAddress')) {
			$return['upload_columns'][] = [
				'name' => 'freeRadius_radiusFramedIPAddress',
				'description' => _('IP address'),
				'help' => 'radiusFramedIPAddress',
				'example' => '123.123.123.123',
			];
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusFramedIPNetmask')) {
			$return['upload_columns'][] = [
				'name' => 'freeRadius_radiusFramedIPNetmask',
				'description' => _('Net mask'),
				'help' => 'radiusFramedIPNetmask',
				'example' => '255.255.255.0'
			];
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusExpiration')) {
			$return['upload_columns'][] = [
				'name' => 'freeRadius_radiusExpiration',
				'description' => _('Expiration date'),
				'help' => 'radiusExpiration',
				'example' => '2035-11-22 00:00'
			];
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusIdleTimeout')) {
			$return['upload_columns'][] = [
				'name' => 'freeRadius_radiusIdleTimeout',
				'description' => _('Idle timeout'),
				'help' => 'radiusIdleTimeout',
				'example' => '3600'
			];
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideDialupAccess')) {
			$return['upload_columns'][] = [
				'name' => 'freeRadius_dialupAccess',
				'description' => _('Enabled'),
				'help' => 'dialupAccess',
				'example' => 'true',
				'values' => 'true, false'
			];
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusProfileDn')) {
			$return['upload_columns'][] = [
				'name' => 'freeRadius_radiusProfileDn',
				'description' => _('Profile'),
				'help' => 'radiusProfileDn',
				'example' => 'cn=profile,ou=radiusProfile,dc=example,dc=com'
			];
		}
		// available PDF fields
		$return['PDF_fields'] = [];
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusFramedIPAddress')) {
			$return['PDF_fields']['radiusFramedIPAddress'] = _('IP address');
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusFramedIPNetmask')) {
			$return['PDF_fields']['radiusFramedIPNetmask'] = _('Net mask');
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusRealm')) {
			$return['PDF_fields']['radiusRealm'] = _('Realm');
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusGroupName')) {
			$return['PDF_fields']['radiusGroupName'] = _('Group names');
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusExpiration')) {
			$return['PDF_fields']['radiusExpiration'] = _('Expiration date');
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusIdleTimeout')) {
			$return['PDF_fields']['radiusIdleTimeout'] = _('Idle timeout');
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideDialupAccess')) {
			$return['PDF_fields']['dialupAccess'] = _('Enabled');
		}
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusProfileDn')) {
			$return['PDF_fields']['radiusProfileDn'] = _('Profile');
		}
		return $return;
	}

	/**
	 * Returns a list of configuration options.
	 *
	 * Calling this method does not require the existence of an enclosing {@link accountContainer}.<br>
	 * <br>
	 * The field names are used as keywords to load and save settings.
	 * We recommend to use the module name as prefix for them (e.g. posixAccount_homeDirectory) to avoid naming conflicts.
	 *
	 * @param array $scopes account types (user, group, host)
	 * @param array $allScopes list of all active account modules and their scopes (module => array(scopes))
	 * @return mixed htmlElement or array of htmlElement
	 *
	 * @see htmlElement
	 */
	public function get_configOptions($scopes, $allScopes) {
		$configContainer = new htmlResponsiveRow();
		$configContainer->add(new htmlResponsiveInputField(_('Profile DN'), 'freeRadius_profileDN', '', 'profileDN'), 12);
		$configContainer->addVerticalSpacer('1rem');
		$hiddenGroup = new htmlGroup();
		$hiddenGroup->addElement(new htmlOutputText(_('Hidden options')));
		$hiddenGroup->addElement(new htmlHelpLink('hiddenOptions'));
		$configContainer->add($hiddenGroup, 12);
		$configContainer->addVerticalSpacer('0.5rem');
		$configContainer->add(new htmlResponsiveInputCheckbox('freeRadius_hideRadiusFramedIPAddress', false, _('IP address'), null, true), 12, 4);
		$configContainer->add(new htmlResponsiveInputCheckbox('freeRadius_hideRadiusFramedIPNetmask', false, _('Net mask'), null, true), 12, 4);
		$configContainer->add(new htmlResponsiveInputCheckbox('freeRadius_hideRadiusRealm', false, _('Realm'), null, true), 12, 4);
		$configContainer->add(new htmlResponsiveInputCheckbox('freeRadius_hideRadiusGroupName', false, _('Group names'), null, true), 12, 4);
		$configContainer->add(new htmlResponsiveInputCheckbox('freeRadius_hideRadiusExpiration', false, _('Expiration date'), null, true), 12, 4);
		$configContainer->add(new htmlResponsiveInputCheckbox('freeRadius_hideRadiusIdleTimeout', false, _('Idle timeout'), null, true), 12, 4);
		$configContainer->add(new htmlResponsiveInputCheckbox('freeRadius_hideRadiusProfileDn', false, _('Profile'), null, true), 12, 4);
		$configContainer->add(new htmlResponsiveInputCheckbox('freeRadius_hideDialupAccess', false, _('Enabled'), null, true), 12, 4);
		return $configContainer;
	}

	/**
	 * This function fills the error message array with messages
	 */
	function load_Messages() {
		$this->messages['radiusFramedIPAddress'][0] = ['ERROR', _('The IP address is invalid.')];
		$this->messages['radiusFramedIPAddress'][1] = ['ERROR', _('Account %s:') . ' freeRadius_radiusFramedIPAddress', _('The IP address is invalid.')];
		$this->messages['radiusFramedIPNetmask'][0] = ['ERROR', _('The net mask is invalid.')];
		$this->messages['radiusFramedIPNetmask'][1] = ['ERROR', _('Account %s:') . ' freeRadius_radiusFramedIPNetmask', _('The net mask is invalid.')];
		$this->messages['radiusRealm'][0] = ['ERROR', _('Please enter a valid realm.')];
		$this->messages['radiusRealm'][1] = ['ERROR', _('Account %s:') . ' freeRadius_radiusRealm', _('Please enter a valid realm.')];
		$this->messages['radiusGroupName'][0] = ['ERROR', _('Please enter a valid list of group names.')];
		$this->messages['radiusGroupName'][1] = ['ERROR', _('Account %s:') . ' freeRadius_radiusGroupName', _('Please enter a valid list of group names.')];
		$this->messages['radiusExpiration'][0] = ['ERROR', _('The expiration date must be in format DD.MM.YYYY HH:MM.')];
		$this->messages['radiusExpiration'][1] = ['ERROR', _('Account %s:') . ' freeRadius_radiusExpiration', _('The expiration date must be in format DD.MM.YYYY HH:MM.')];
		$this->messages['radiusIdleTimeout'][0] = ['ERROR', _('Please enter a numeric value for the idle timeout.')];
		$this->messages['radiusIdleTimeout'][1] = ['ERROR', _('Account %s:') . ' freeRadius_radiusIdleTimeout', _('Please enter a numeric value for the idle timeout.')];
		$this->messages['dialupAccess'][0] = ['ERROR', _('Account %s:') . ' freeRadius_dialupAccess', _('This value can only be "true" or "false".')];
		$this->messages['radiusProfileDn'][0] = ['ERROR', _('Account %s:') . ' freeRadius_radiusProfileDn', _('This is not a valid DN!')];
	}

	/**
	 * {@inheritDoc}
	 */
	public function loadAttributesFromAccountCopy(array $ldapAttributes, array $attributesToIgnore = []): void {
		$attributesToIgnore = array_merge(baseModule::ATTRIBUTES_TO_IGNORE_ON_COPY_DEFAULT, self::ATTRIBUTES_TO_IGNORE_ON_COPY);
		parent::loadAttributesFromAccountCopy($ldapAttributes, $attributesToIgnore);
	}

	/**
	 * Returns the HTML meta data for the main account page.
	 *
	 * @return htmlElement HTML meta data
	 */
	function display_html_attributes() {
		$return = new htmlResponsiveRow();
		if (in_array('radiusprofile', $this->attributes['objectClass'])) {
			// realm
			if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusRealm')) {
				$this->addSimpleInputTextField($return, 'radiusRealm', _('Realm'));
			}
			// group names
			if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusGroupName')) {
				$this->addMultiValueInputTextField($return, 'radiusGroupName', _('Group names'));
			}
			// IP address
			if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusFramedIPAddress')) {
				$this->addSimpleInputTextField($return, 'radiusFramedIPAddress', _('IP address'));
			}
			// net mask
			if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusFramedIPNetmask')) {
				$this->addSimpleInputTextField($return, 'radiusFramedIPNetmask', _('Net mask'));
			}
			// idle timeout
			if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusIdleTimeout')) {
				$radiusIdleTimeoutInput = $this->addSimpleInputTextField($return, 'radiusIdleTimeout', _('Idle timeout'));
				$radiusIdleTimeoutInput->setMinimumAndMaximumNumber();
			}
			// expiration date
			if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusExpiration')) {
				$radiusExpiration = '&nbsp;&nbsp;-';
				if (isset($this->attributes['radiusExpiration'][0])) {
					$radiusExpiration = $this->formatExpirationDate($this->attributes['radiusExpiration'][0]);
				}
				$return->addLabel(new htmlOutputText('Expiration date'));
				$radiusExpirationList = new htmlGroup();
				$radiusExpirationList->addElement(new htmlOutputText($radiusExpiration . ' &nbsp;&nbsp;&nbsp;&nbsp;', false));
				$radiusExpirationList->addElement(new htmlAccountPageButton(get_class($this), 'expiration', 'change', _('Change')));
				$radiusExpirationList->addElement(new htmlHelpLink('radiusExpiration'));
				$return->addField($radiusExpirationList);
			}
			// profile DN
			if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusProfileDn')) {
				$profiles = ['-' => '-'];
				foreach ($this->getProfiles() as $dn) {
					$profiles[getAbstractDN($dn)] = $dn;
				}
				$profile = [];
				if (!empty($this->attributes['radiusProfileDn'][0])) {
					$profile = $this->attributes['radiusProfileDn'];
					if (!in_array($this->attributes['radiusProfileDn'][0], $profiles)) {
						$profiles[getAbstractDN($this->attributes['radiusProfileDn'][0])] = $this->attributes['radiusProfileDn'][0];
					}
				}
				$profileSelect = new htmlResponsiveSelect('radiusProfileDn', $profiles, $profile, _('Profile'), 'radiusProfileDn');
				$profileSelect->setHasDescriptiveElements(true);
				$return->add($profileSelect, 12);
			}
			// enabled
			if (!$this->isBooleanConfigOptionSet('freeRadius_hideDialupAccess')) {
				$enabled = [''];
				if (!empty($this->attributes['dialupAccess'][0])) {
					$enabled = [$this->attributes['dialupAccess'][0]];
					// value in LDAP may be anything other than "false" to count as "true"
					if (!in_array($this->attributes['dialupAccess'][0], ['true', 'false', 'TRUE', 'FALSE'])) {
						$enabled = ['true'];
					}
				}
				$enabledOptions = ['-' => '', _('Yes') => 'true', _('No') => 'false'];
				$enabledSelect = new htmlResponsiveSelect('dialupAccess', $enabledOptions, $enabled, _('Enabled'), 'dialupAccess');
				$enabledSelect->setHasDescriptiveElements(true);
				$return->add($enabledSelect, 12);
			}
			// button to remove extension
			$return->addVerticalSpacer('2rem');
			$remButton = new htmlButton('remObjectClass', _('Remove FreeRadius extension'));
			$remButton->setCSSClasses(['lam-danger']);
			$return->add($remButton, 12, 12, 12, 'text-center');
		}
		else {
			$return->add(new htmlButton('addObjectClass', _('Add FreeRadius extension')), 12);
		}
		return $return;
	}

	/**
	 * Processes user input of the primary module page.
	 * It checks if all input values are correct and updates the associated LDAP attributes.
	 *
	 * @return array list of info/error messages
	 */
	function process_attributes() {
		if (isset($_POST['addObjectClass'])) {
			$this->attributes['objectClass'][] = 'radiusprofile';
			return [];
		}
		elseif (isset($_POST['remObjectClass'])) {
			$this->attributes['objectClass'] = array_delete(['radiusprofile'], $this->attributes['objectClass']);
			for ($i = 0; $i < sizeof($this->meta['attributes']); $i++) {
				if (isset($this->attributes[$this->meta['attributes'][$i]])) {
					unset($this->attributes[$this->meta['attributes'][$i]]);
				}
			}
			return [];
		}
		// skip processing if extension is not active
		if (!in_array('radiusprofile', $this->attributes['objectClass'])) {
			return [];
		}
		$errors = [];
		// IP address
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusFramedIPAddress')) {
			$this->attributes['radiusFramedIPAddress'][0] = $_POST['radiusFramedIPAddress'];
			if (($_POST['radiusFramedIPAddress'] != '') && !get_preg($_POST['radiusFramedIPAddress'], 'ip')) {
				$errors[] = $this->messages['radiusFramedIPAddress'][0];
			}
		}
		// net mask
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusFramedIPNetmask')) {
			$this->attributes['radiusFramedIPNetmask'][0] = $_POST['radiusFramedIPNetmask'];
			if (($_POST['radiusFramedIPNetmask'] != '') && !get_preg($_POST['radiusFramedIPNetmask'], 'ip')) {
				$errors[] = $this->messages['radiusFramedIPNetmask'][0];
			}
		}
		// realm
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusRealm')) {
			$this->attributes['radiusRealm'][0] = $_POST['radiusRealm'];
			if (($_POST['radiusRealm'] != '') && !get_preg($_POST['radiusRealm'], 'DNSname')) {
				$errors[] = $this->messages['radiusRealm'][0];
			}
		}
		// group names
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusGroupName')) {
			$this->processMultiValueInputTextField('radiusGroupName', $errors, 'groupname');
		}
		// idle timeout
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusIdleTimeout')) {
			$this->attributes['radiusIdleTimeout'][0] = $_POST['radiusIdleTimeout'];
			if (($_POST['radiusIdleTimeout'] != '') && !get_preg($_POST['radiusIdleTimeout'], 'digit')) {
				$errors[] = $this->messages['radiusIdleTimeout'][0];
			}
		}
		// enabled
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideDialupAccess')) {
			if (!empty($this->attributes['dialupAccess']) && ($_POST['dialupAccess'] === '')) {
				unset($this->attributes['dialupAccess']);
			}
			elseif ($_POST['dialupAccess'] === 'false') {
				$this->attributes['dialupAccess'][0] = 'false';
			}
			elseif ($_POST['dialupAccess'] === 'true') {
				$this->attributes['dialupAccess'][0] = 'true';
			}
		}
		// profile DN
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusProfileDn')) {
			if (($_POST['radiusProfileDn'] == '-') && !empty($this->attributes['radiusProfileDn'])) {
				unset($this->attributes['radiusProfileDn']);
			}
			elseif ($_POST['radiusProfileDn'] != '-') {
				$this->attributes['radiusProfileDn'][0] = $_POST['radiusProfileDn'];
			}
		}
		return $errors;
	}

	/**
	 * This function will create the meta HTML code to show a page to change the expiration date.
	 *
	 * @return htmlElement meta HTML code
	 */
	function display_html_expiration() {
		$return = new htmlResponsiveRow();
		$attr = 'radiusExpiration';
		$date = new DateTime('@' . (time() + 3600 * 24 * 365), getTimeZone());
		if (!empty($this->attributes[$attr][0])) {
			$date = DateTime::createFromFormat('d M Y H:i', $this->attributes[$attr][0], new DateTimeZone('UTC'));
			$date->setTimezone(getTimeZone());
		}
		$dateInput = new htmlResponsiveInputField(_('Expiration date'), 'expirationTime', $date->format('Y-m-d H:i'), 'radiusExpiration');
		$dateInput->showCalendar('Y-m-d H:i', true);
		$return->add($dateInput);

		$return->addVerticalSpacer('2rem');
		$buttons = new htmlGroup();
		$buttons->addElement(new htmlAccountPageButton(get_class($this), 'attributes', 'change' . $attr, _('Change')));
		$buttons->addElement(new htmlSpacer('0.5rem', null));
		if (isset($this->attributes[$attr][0])) {
			$buttons->addElement(new htmlAccountPageButton(get_class($this), 'attributes', 'del' . $attr, _('Remove')));
			$buttons->addElement(new htmlSpacer('0.5rem', null));
		}
		$buttons->addElement(new htmlAccountPageButton(get_class($this), 'attributes', 'back' . $attr, _('Cancel')));
		$return->add($buttons, 12);
		return $return;
	}

	/**
	 * Processes user input of the time selection page.
	 *
	 * @return array list of info/error messages
	 */
	function process_expiration() {
		$return = [];
		// find button name
		$buttonName = '';
		$postKeys = array_keys($_POST);
		for ($i = 0; $i < sizeof($postKeys); $i++) {
			if (strpos($postKeys[$i], 'form_subpage_freeRadius_attributes_') !== false) {
				$buttonName = $postKeys[$i];
			}
		}
		if (($buttonName == '') || (strpos($buttonName, '_back') !== false)) {
			return [];
		}
		// get attribute name
		$attr = '';
		if (strpos($buttonName, 'radiusExpiration') !== false) {
			$attr = 'radiusExpiration';
		}
		if ($attr == '') {
			return [];
		}
		// determine action
		if (strpos($buttonName, '_change') !== false) {
			// set new expiration date
			$date = DateTime::createFromFormat('Y-m-d H:i', $_POST['expirationTime'], getTimeZone());
			$date->setTimezone(new DateTimeZone('UTC'));
			$this->attributes[$attr][0] = $date->format('d M Y H:i');
		}
		elseif (strpos($buttonName, '_del') !== false) {
			// remove attribute value
			unset($this->attributes[$attr]);
		}
		return $return;
	}

	/**
	 * Returns a list of modifications which have to be made to the LDAP account.
	 *
	 * @return array list of modifications
	 * <br>This function returns an array with 3 entries:
	 * <br>array( DN1 ('add' => array($attr), 'remove' => array($attr), 'modify' => array($attr)), DN2 .... )
	 * <br>DN is the DN to change. It may be possible to change several DNs (e.g. create a new user and add him to some groups via attribute memberUid)
	 * <br>"add" are attributes which have to be added to LDAP entry
	 * <br>"remove" are attributes which have to be removed from LDAP entry
	 * <br>"modify" are attributes which have to been modified in LDAP entry
	 * <br>"info" are values with informational value (e.g. to be used later by pre/postModify actions)
	 */
	function save_attributes() {
		if (!in_array('radiusprofile', $this->attributes['objectClass']) && !in_array('radiusprofile', $this->orig['objectClass'])) {
			// skip saving if the extension was not added/modified
			return [];
		}
		return parent::save_attributes();
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::build_uploadAccounts()
	 */
	function build_uploadAccounts($rawAccounts, $ids, &$partialAccounts, $selectedModules, &$type) {
		$errors = [];
		for ($i = 0; $i < sizeof($rawAccounts); $i++) {
			// add object class
			if (!in_array("radiusprofile", $partialAccounts[$i]['objectClass'])) {
				$partialAccounts[$i]['objectClass'][] = "radiusprofile";
			}
			// IP address
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'freeRadius_radiusFramedIPAddress', 'radiusFramedIPAddress',
				'ip', $this->messages['radiusFramedIPAddress'][1], $errors);
			// net mask
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'freeRadius_radiusFramedIPNetmask', 'radiusFramedIPNetmask',
				'ip', $this->messages['radiusFramedIPNetmask'][1], $errors);
			// realm
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'freeRadius_radiusRealm', 'radiusRealm',
				'DNSname', $this->messages['radiusRealm'][1], $errors);
			// group names
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'freeRadius_radiusGroupName', 'radiusGroupName', 'groupname', $this->messages['radiusGroupName'][1], $errors, '/;[ ]*/');
			// expiration date
			if ($rawAccounts[$i][$ids['freeRadius_radiusExpiration']] != "") {
				$date = DateTime::createFromFormat('Y-m-d H:i', $rawAccounts[$i][$ids['freeRadius_radiusExpiration']], getTimeZone());
				if ($date !== false) {
					$date->setTimezone(new DateTimeZone('UTC'));
					$partialAccounts[$i]['radiusExpiration'] = $date->format('d M Y H:i');;
				}
				else {
					$errMsg = $this->messages['radiusExpiration'][1];
					array_push($errMsg, [$i]);
					$errors[] = $errMsg;
				}
			}
			// idle timeout
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'freeRadius_radiusIdleTimeout', 'radiusIdleTimeout',
				'digit', $this->messages['radiusIdleTimeout'][1], $errors);
			// enabled
			if (!empty($rawAccounts[$i][$ids['freeRadius_dialupAccess']])) {
				if (in_array($rawAccounts[$i][$ids['freeRadius_dialupAccess']], ['true', 'false'])) {
					$partialAccounts[$i]['dialupAccess'] = $rawAccounts[$i][$ids['freeRadius_dialupAccess']];
				}
				else {
					$errMsg = $this->messages['dialupAccess'][0];
					array_push($errMsg, [$i]);
					$errors[] = $errMsg;
				}
			}
			// profile DN
			$this->mapSimpleUploadField($rawAccounts, $ids, $partialAccounts, $i, 'freeRadius_radiusProfileDn', 'radiusProfileDn', 'dn', $this->messages['radiusProfileDn'][0], $errors);
		}
		return $errors;
	}

	/**
	 * {@inheritDoc}
	 * @see baseModule::get_pdfEntries()
	 */
	function get_pdfEntries($pdfKeys, $typeId) {
		$return = [];
		$this->addSimplePDFField($return, 'radiusFramedIPAddress', _('IP address'));
		$this->addSimplePDFField($return, 'radiusFramedIPNetmask', _('Net mask'));
		$this->addSimplePDFField($return, 'radiusRealm', _('Realm'));
		$this->addSimplePDFField($return, 'radiusGroupName', _('Group names'));
		$this->addSimplePDFField($return, 'radiusIdleTimeout', _('Idle timeout'));
		$this->addSimplePDFField($return, 'radiusProfileDn', _('Profile'));
		if (isset($this->attributes['radiusExpiration'][0])) {
			$this->addPDFKeyValue($return, 'radiusExpiration', _('Expiration date'), $this->formatExpirationDate($this->attributes['radiusExpiration'][0]));
		}
		if (isset($this->attributes['dialupAccess'][0])) {
			$enabled = _('Yes');
			if (in_array($this->attributes['dialupAccess'][0], ['false', 'FALSE'])) {
				$enabled = _('No');
			}
			$this->addPDFKeyValue($return, 'dialupAccess', _('Enabled'), $enabled);
		}
		return $return;
	}

	/**
	 * {@inheritDoc}
	 */
	function check_profileOptions($options, $typeId) {
		$messages = parent::check_profileOptions($options, $typeId);
		// group names
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusGroupName')
			&& isset($options['freeRadius_radiusGroupName'][0])
			&& ($options['freeRadius_radiusGroupName'][0] != '')) {
			$list = preg_split('/;[ ]*/', $options['freeRadius_radiusGroupName'][0]);
			for ($i = 0; $i < sizeof($list); $i++) {
				if (!get_preg($list[$i], 'groupname')) {
					$messages[] = $this->messages['radiusGroupName'][0];
					break;
				}
			}
		}
		return $messages;
	}

	/**
	 * Loads the values of an account profile into internal variables.
	 *
	 * @param array $profile hash array with profile values (identifier => value)
	 */
	function load_profile($profile) {
		// profile mappings in meta data
		parent::load_profile($profile);
		// add extension
		if (isset($profile['freeRadius_addExt'][0]) && ($profile['freeRadius_addExt'][0] === "true")) {
			if (!in_array('radiusprofile', $this->attributes['objectClass'])) {
				$this->attributes['objectClass'][] = 'radiusprofile';
			}
		}
		// group names
		if (!$this->isBooleanConfigOptionSet('freeRadius_hideRadiusGroupName')
			&& isset($profile['freeRadius_radiusGroupName'][0])
			&& ($profile['freeRadius_radiusGroupName'][0] != '')) {
			$this->attributes['radiusGroupName'] = preg_split('/;[ ]*/', $profile['freeRadius_radiusGroupName'][0]);
		}
	}

	/**
	 * Formats the expiration date attribute.
	 *
	 * @param String $date date value
	 */
	private function formatExpirationDate($date) {
		if (is_null($date) || ($date == '')) {
			return $date;
		}
		$dateTime = DateTime::createFromFormat('d M Y H:i', $date, new DateTimeZone('UTC'));
		$dateTime->setTimezone(getTimeZone());
		return $dateTime->format('Y-m-d H:i');
	}

	/**
	 * Returns a list of possible profile DNs.
	 *
	 * @return array list of profile DNs
	 */
	private function getProfiles() {
		if ($this->profileCache != null) {
			return $this->profileCache;
		}
		if (empty($this->moduleSettings['freeRadius_profileDN'][0])) {
			return [];
		}
		$list = searchLDAP($this->moduleSettings['freeRadius_profileDN'][0], '(objectClass=radiusProfile)', ['dn']);
		if (empty($list)) {
			return [];
		}
		foreach ($list as $attr) {
			$this->profileCache[] = $attr['dn'];
		}
		usort($this->profileCache, 'compareDN');
		return $this->profileCache;
	}

	/**
	 * Returns a list of jobs that can be run.
	 *
	 * @param LAMConfig $config configuration
	 * @return array list of jobs
	 */
	public function getSupportedJobs(&$config) {
		return [
			new FreeRadiusAccountExpirationCleanupJob(),
			new FreeRadiusAccountExpirationNotifyJob()
		];
	}

	/**
	 * @inheritDoc
	 */
	public function getListAttributeDescriptions(ConfiguredType $type): array {
		return [
			'radiusframedipaddress' => _('IP address'),
			'radiusframedipnetmask' => _('Net mask'),
			'radiusrealm' => _('Realm'),
			'radiusgroupname' => _('Group names'),
			'radiusexpiration' => _('Expiration date'),
			'radiusidletimeout' => _('Idle timeout'),
			'dialupaccess' => _('Enabled'),
			'radiusprofiledn' => _('Profile'),
		];
	}

}

if (interface_exists('\LAM\JOB\Job', false)) {

	include_once __DIR__ . '/../passwordExpirationJob.inc';

	/**
	 * Job to delete or move users on account expiration.
	 *
	 * @package jobs
	 */
	class FreeRadiusAccountExpirationCleanupJob extends \LAM\JOB\AccountExpirationCleanupJob {

		/**
		 * Returns the alias name of the job.
		 *
		 * @return String name
		 */
		public function getAlias() {
			return _('FreeRadius') . ': ' . _('Cleanup expired user accounts');
		}

		/**
		 * Returns the description of the job.
		 *
		 * @return String description
		 */
		public function getDescription() {
			return _('This job deletes or moves user accounts when they expire.');
		}

		/**
		 * Searches for users in LDAP.
		 *
		 * @param String $jobID unique job identifier
		 * @param array $options config options (name => value)
		 * @return array list of user attributes
		 */
		protected function findUsers($jobID, $options) {
			// read users
			$attrs = ['radiusExpiration'];
			return searchLDAPByFilter('(radiusExpiration=*)', $attrs, ['user']);
		}

		/**
		 * Checks if a user is expired.
		 *
		 * @param integer $jobID job ID
		 * @param array $options job settings
		 * @param PDO $pdo PDO
		 * @param DateTime $now current time
		 * @param array $policyOptions list of policy options by getPolicyOptions()
		 * @param array $user user attributes
		 * @param boolean $isDryRun just do a dry run, nothing is modified
		 */
		protected function checkSingleUser($jobID, $options, &$pdo, $now, $policyOptions, $user, $isDryRun) {
			$expireTime = DateTime::createFromFormat('d M Y H:i', $user['radiusexpiration'][0], new DateTimeZone('UTC'));
			$this->jobResultLog->logDebug("Expiration on " . $expireTime->format('Y-m-d'));
			$delay = 0;
			if (!empty($options[$this->getConfigPrefix() . '_delay' . $jobID][0])) {
				$delay = $options[$this->getConfigPrefix() . '_delay' . $jobID][0];
			}
			$actionTime = clone $expireTime;
			if ($delay != 0) {
				$actionTime->add(new DateInterval('P' . $delay . 'D'));
			}
			$actionTime->setTimeZone(getTimeZone());
			$this->jobResultLog->logDebug("Action time on " . $actionTime->format('Y-m-d'));
			if ($actionTime <= $now) {
				$this->performAction($jobID, $options, $user, $isDryRun);
			}
		}

	}

	/**
	 * Job to notify users about account expiration.
	 *
	 * @package jobs
	 */
	class FreeRadiusAccountExpirationNotifyJob extends \LAM\JOB\PasswordExpirationJob {

		/**
		 * {@inheritDoc}
		 * @see \LAM\JOB\Job::getAlias()
		 */
		public function getAlias() {
			return _('FreeRadius') . ': ' . _('Notify users about account expiration');
		}

		/**
		 * {@inheritDoc}
		 * @see \LAM\JOB\PasswordExpirationJob::getDescription()
		 */
		public function getDescription() {
			return _('This job sends out emails to inform your users that their account will expire soon.');
		}

		/**
		 * {@inheritDoc}
		 * @see \LAM\JOB\PasswordExpirationJob::findUsers()
		 */
		protected function findUsers($jobID, $options) {
			// read users
			$sysattrs = ['radiusExpiration', $_SESSION['cfgMain']->getMailAttribute()];
			$attrs = $this->getAttrWildcards($jobID, $options);
			$attrs = array_values(array_unique(array_merge($attrs, $sysattrs)));
			return searchLDAPByFilter('(&(radiusExpiration=*)(' . $_SESSION['cfgMain']->getMailAttribute() . '=*))', $attrs, ['user']);
		}

		/**
		 * {@inheritDoc}
		 * @see \LAM\JOB\PasswordExpirationJob::checkSingleUser()
		 */
		protected function checkSingleUser($jobID, $options, &$pdo, $now, $policyOptions, $user, $isDryRun) {
			$dn = $user['dn'];
			// get time when account expires
			$expirationTime = DateTime::createFromFormat('d M Y H:i', $user['radiusexpiration'][0], new DateTimeZone('UTC'));
			$this->jobResultLog->logDebug("Account expiration on " . $expirationTime->format('Y-m-d'));
			// skip if account itself is expired
			if ($expirationTime <= $now) {
				$this->jobResultLog->logDebug($dn . ' already expired');
				return;
			}
			$numDaysToWarn = $options[$this->getConfigPrefix() . '_mailNotificationPeriod' . $jobID][0];
			$this->jobResultLog->logDebug("Number of days before warning " . $numDaysToWarn);
			// calculate time of notification
			$notifyTime = clone $expirationTime;
			$notifyTime->sub(new DateInterval('P' . $numDaysToWarn . 'D'));
			$notifyTime->setTimeZone(getTimeZone());
			$this->jobResultLog->logDebug("Account expiration notification on " . $notifyTime->format('Y-m-d H:i'));
			// skip if notification is in the future
			if ($notifyTime > $now) {
				$this->jobResultLog->logDebug($dn . ' does not need notification yet.');
				return;
			}
			$dbLastChange = $this->getDBLastPwdChangeTime($jobID, $pdo, $dn);
			// skip entries where mail was already sent
			if ($dbLastChange == $user['radiusexpiration'][0]) {
				$this->jobResultLog->logDebug($dn . ' was already notified.');
				return;
			}
			if ($isDryRun) {
				// no action for dry run
				$this->jobResultLog->logInfo('Not sending email to ' . $dn . ' because of dry run.');
				return;
			}
			// send email
			$success = $this->sendMail($options, $jobID, $user, $expirationTime);
			// update DB if mail was sent successfully
			if ($success) {
				$this->setDBLastPwdChangeTime($jobID, $pdo, $dn, $user['radiusexpiration'][0]);
			}
		}

	}

}
